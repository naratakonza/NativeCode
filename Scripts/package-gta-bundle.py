"""Audit and package the pinned local GTA iOS bundle without changing its bytes."""

import argparse
import copy
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import plistlib
import posixpath
import shutil
import stat
import struct
import sys
import tempfile
import zipfile

PREFIX = "Payload/gtasa.app/"
CHUNK = 1024 * 1024
SYSTEM = ("/usr/lib/", "/System/Library/")
LOADS = {0xC, 0x20, 0x80000018, 0x8000001F, 0x80000023}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def stream_hash(reader, output=None):
    digest = hashlib.sha256()
    while chunk := reader.read(CHUNK):
        digest.update(chunk)
        if output is not None:
            output.write(chunk)
    return digest.hexdigest()


def plain(path, kind):
    info = path.lstat()
    require(stat.S_IFMT(info.st_mode) == kind and not (
        getattr(info, "st_file_attributes", 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT
    ), f"Not a regular file/directory: {path}")
    return info


def file_hash(path):
    before = plain(path, stat.S_IFREG)
    with path.open("rb") as reader:
        digest = stream_hash(reader)
    after = plain(path, stat.S_IFREG)
    require((before.st_size, before.st_mtime_ns) == (after.st_size, after.st_mtime_ns),
            f"File changed while reading: {path}")
    return digest


def source_files(root):
    plain(root, stat.S_IFDIR)
    files = set()
    for directory, folders, names in os.walk(root, followlinks=False):
        for name in folders:
            plain(Path(directory, name), stat.S_IFDIR)
        for name in names:
            path = Path(directory, name)
            plain(path, stat.S_IFREG)
            files.add(path.relative_to(root).as_posix())
    return files


def archive_files(archive):
    members = {}
    folded = set()
    for member in archive.infolist():
        name = member.filename.removeprefix(PREFIX)
        mode = member.external_attr >> 16
        require(member.filename.startswith(PREFIX) and name and
                str(PurePosixPath(name)) == name and
                not any(x in name for x in ("\\", ":")) and
                all(x not in ("", ".", "..") for x in name.split("/")) and
                not member.is_dir() and stat.S_ISREG(mode) and not member.flag_bits & 1,
                f"Unexpected ZIP entry: {member.filename}")
        require(name.casefold() not in folded, f"Duplicate ZIP entry: {name}")
        folded.add(name.casefold())
        members[name] = member
    return members


def native_metadata(path, expected_type):
    data = path.read_bytes()
    require(len(data) >= 32, f"Truncated Mach-O: {path.name}")
    magic, cpu, subtype, kind, count, size = struct.unpack_from("<6I", data)
    require((magic, cpu, kind) == (0xFEEDFACF, 0x0100000C, expected_type)
            and subtype in (0, 1), f"Expected thin iPhoneOS ARM64 Mach-O: {path.name}")
    offset, end = 32, 32 + size
    require(end <= len(data) and count <= size // 8, f"Invalid load command table: {path.name}")
    result = {"type": "MH_EXECUTE" if kind == 2 else "MH_DYLIB", "architecture": "arm64",
              "rpaths": [], "dependencies": [], "install_name": None,
              "signature_blob_sha256": None, "platform": None, "cryptid": None}
    for _ in range(count):
        tag, length = struct.unpack_from("<II", data, offset)
        require(length >= 8 and length % 8 == 0 and offset + length <= end,
                f"Invalid Mach-O load command: {path.name}")
        if tag in LOADS or tag in (0xD, 0x8000001C):
            minimum = 12 if tag == 0x8000001C else 24
            require(length > minimum, f"Missing load-command string: {path.name}")
            relative, = struct.unpack_from("<I", data, offset + 8)
            require(minimum <= relative < length, f"Bad load-command string: {path.name}")
            start = offset + relative
            text = data[start:data.index(0, start, offset + length)].decode("utf-8")
            if tag == 0xD:
                result["install_name"] = text
            elif tag == 0x8000001C:
                result["rpaths"].append(text)
            else:
                result["dependencies"].append({"name": text, "weak": tag == 0x80000018})
        elif tag == 0x32:
            require(length >= 24, f"Truncated LC_BUILD_VERSION: {path.name}")
            result["platform"] = struct.unpack_from("<I", data, offset + 8)[0]
        elif tag == 0x25:
            require(length >= 16, f"Truncated LC_VERSION_MIN_IPHONEOS: {path.name}")
            require(result["platform"] in (None, 2), f"Conflicting Mach-O platform: {path.name}")
            result["platform"] = 2
        elif tag == 0x1D:
            require(length >= 16, f"Truncated signature command: {path.name}")
            start, size = struct.unpack_from("<II", data, offset + 8)
            require(end <= start < len(data) and 0 < size <= len(data) - start,
                    f"Invalid embedded signature range: {path.name}")
            result["signature_blob_sha256"] = hashlib.sha256(data[start:start + size]).hexdigest()
        elif tag in (0x21, 0x2C):
            require(length >= 20, f"Truncated encryption command: {path.name}")
            result["cryptid"] = struct.unpack_from("<I", data, offset + 16)[0]
        offset += length
    require(offset == end and result["platform"] == 2, f"Expected iPhoneOS platform: {path.name}")
    return result


def expand_token(path, loader):
    for prefix, directory in (("@loader_path/", posixpath.dirname(loader)), ("@executable_path/", "")):
        if path.startswith(prefix):
            return posixpath.normpath(posixpath.join(directory, path.removeprefix(prefix)))
    return path


def audit_native(app, config, files):
    executables = {"gtasa": 2}
    for name in config["frameworks"]:
        directory = f"Frameworks/{name}.framework"
        info = plistlib.loads((app / directory / "Info.plist").read_bytes())
        require(info["CFBundleExecutable"] == name, f"Unexpected framework executable: {name}")
        executables[f"{directory}/{name}"] = 6
    executables.update({name: 6 for name in config["preserved_weak_dylibs"]})
    metadata = {name: native_metadata(app / name, kind) for name, kind in executables.items()}
    main = metadata["gtasa"]
    for name in config["frameworks"]:
        install = f"@rpath/{name}.framework/{name}"
        require(metadata[f"Frameworks/{name}.framework/{name}"]["install_name"] == install and
                any(dep["name"] == install and not dep["weak"] for dep in main["dependencies"]),
                f"Missing original framework dependency: {name}")
    for name in config["preserved_weak_dylibs"]:
        require(any(dep["weak"] and dep["name"] == f"@executable_path/{name}"
                    for dep in main["dependencies"]), f"Expected original weak dylib: {name}")
    for name, image in metadata.items():
        paths = [expand_token(p, name) for p in image["rpaths"]] + [
            expand_token(p, "gtasa") for p in main["rpaths"]
        ]
        for dep in image["dependencies"]:
            load = dep["name"]
            candidates = ([posixpath.join(path, load.removeprefix("@rpath/")) for path in paths]
                          if load.startswith("@rpath/") else [expand_token(load, name)])
            bundled = next((p for p in candidates if p in files and p in metadata), None)
            system = next((p for p in candidates if p.startswith(SYSTEM)), None)
            require(bundled or system or dep["weak"], f"Unresolved required dependency: {name} → {load}")
            dep["resolution"] = bundled or system or "missing weak dependency"
    return metadata, set(executables) - set(config["preserved_weak_dylibs"])


def verify_archive(path, manifest):
    with zipfile.ZipFile(path) as archive:
        members = archive_files(archive)
        require(members.keys() == manifest.keys(), "IPA payload file list differs from source manifest")
        for name, member in members.items():
            expected = manifest[name]
            require((member.file_size, member.CRC, member.external_attr >> 16) ==
                    (expected["size"], expected["crc32"], expected["mode"]),
                    f"IPA size, CRC or permission differs: {name}")
            with archive.open(member) as reader:
                require(stream_hash(reader) == expected["sha256"], f"IPA payload SHA-256 differs: {name}")
        # Reading every member to EOF also validates its ZIP CRC.
    return {"path": str(path), "size": path.stat().st_size, "sha256": file_hash(path)}


def write_archive(target, app, original, original_members, manifest):
    with zipfile.ZipFile(target, "w") as output:
        output.comment = original.comment
        for name, entry in original_members.items():
            info = copy.copy(entry)
            info.external_attr = (manifest[name]["mode"] << 16) | (entry.external_attr & 0xFFFF)
            # ZipFile.open uses the copied ZipInfo's per-member compression level.
            info._compresslevel = 1
            plain(app / name, stat.S_IFREG)
            with (app / name).open("rb") as reader, output.open(info, "w") as writer:
                require(stream_hash(reader, writer) == manifest[name]["sha256"],
                        f"Source changed during packaging: {name}")


def build_archive(destination, app, original, original_members, manifest):
    require(not destination.exists() and not destination.is_symlink(), f"Output exists: {destination}")
    free = shutil.disk_usage(destination.parent).free
    estimate = sum(row["size"] for row in manifest.values()) + 32 * CHUNK
    require(free >= max(15_000_000_000, estimate), "Not enough free space under storage policy")
    temp = tempfile.NamedTemporaryFile(prefix=destination.stem + ".", suffix=".partial",
                                       dir=destination.parent, delete=False)
    partial = Path(temp.name)
    temp.close()
    try:
        write_archive(partial, app, original, original_members, manifest)
        result = verify_archive(partial, manifest)
        # Link creation is atomic and refuses to replace an existing IPA on NTFS.
        os.link(partial, destination)
        result["path"] = str(destination)
        return result
    finally:
        partial.unlink()


def run(config, workspace, report):
    def path(key):
        value = workspace / config[key]
        require(value.resolve() == value and value.is_relative_to(workspace), f"Invalid path: {value}")
        return value

    app, original, verified, destination = (
        path(key) for key in ("source_app", "original_ipa", "verified_ipa", "output_ipa")
    )
    plain(destination.parent, stat.S_IFDIR)
    report["source_app"] = str(app)
    report["free_before"] = shutil.disk_usage(workspace).free
    require(file_hash(original) == config["original_sha256"], "Original IPA SHA-256 mismatch")
    with zipfile.ZipFile(original) as baseline:
        members = archive_files(baseline)
        require(len(members) == config["member_count"], "Unexpected original IPA file count")
        files = source_files(app)
        report["missing"] = sorted(members.keys() - files)
        extras = files - members.keys()
        report["excluded_local_analysis"] = sorted(extras & set(config["exclude_local_analysis"]))
        report["unexpected"] = sorted(extras - set(config["exclude_local_analysis"]))
        report["damaged"] = []
        require(not report["missing"] and not report["unexpected"], "Source bundle file list differs")
        manifest = {}
        for name, member in members.items():
            with baseline.open(member) as reader:
                expected = stream_hash(reader)
            if file_hash(app / name) != expected:
                report["damaged"].append(name)
            manifest[name] = {"size": member.file_size, "crc32": member.CRC,
                              "sha256": expected, "mode": member.external_attr >> 16}
        require(not report["damaged"], "Source bundle content differs")
        require(set(config["required_resources"]) <= files, "Missing expected game resources")
        info = plistlib.loads((app / "Info.plist").read_bytes())
        require((info["CFBundleIdentifier"], info["CFBundleExecutable"], info["CFBundlePackageType"])
                == (config["bundle_id"], "gtasa", "APPL"), "Unexpected application identity")
        require(info["UIMainStoryboardFile"] == "Main", "Unexpected startup storyboard")
        metadata, executable_modes = audit_native(app, config, files)
        for name in executable_modes:
            manifest[name]["mode"] |= stat.S_IXUSR
        report.update(bundle_id=info["CFBundleIdentifier"], minimum_os=info.get("MinimumOSVersion"),
                      members=manifest, native=metadata,
                      signing={"status": "ORIGINAL_METADATA_PRESERVED_NOT_NEWLY_SIGNED",
                               "embedded_mobileprovision": "embedded.mobileprovision" in files,
                               "apple_validation": "NOT_TESTED",
                               "device_validation": "NOT_TESTED_FOR_THIS_RUN"})
        if verified.exists():
            result = verify_archive(verified, manifest)
            require(result["sha256"] == config["verified_sha256"], "Verified IPA SHA-256 mismatch")
            report["artifact"] = result
            report["artifact_action"] = "REUSED_VERIFIED"
        elif destination.exists():
            report["artifact"] = verify_archive(destination, manifest)
            report["artifact_action"] = "REUSED_CURRENT"
        else:
            report["artifact"] = build_archive(destination, app, baseline, members, manifest)
            report["artifact_action"] = "BUILT_FROM_SOURCE"
    require(file_hash(original) == config["original_sha256"], "Original IPA changed during run")
    report["free_after"] = shutil.disk_usage(workspace).free


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workspace", type=Path, required=True)
    parser.add_argument("--config", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    workspace = args.workspace.resolve(strict=True)
    report_path = args.report.absolute()
    require(report_path.resolve() == report_path and report_path.is_relative_to(workspace),
            "Report must be inside the workspace")
    plain(report_path.parent, stat.S_IFDIR)
    report = {"packaging": "FAIL"}
    with report_path.open("x", encoding="utf-8") as output:
        try:
            run(json.loads(args.config.read_text(encoding="utf-8")), workspace, report)
            report["packaging"] = "PASS"
        except (OSError, ValueError, KeyError, TypeError, struct.error, zipfile.BadZipFile) as error:
            report["error"] = str(error)
        json.dump(report, output, indent=2)
        output.write("\n")
    print(f'Packaging {report["packaging"]} | {report_path}')
    if report["packaging"] == "FAIL":
        print(report["error"], file=sys.stderr)
        return 1
    print(json.dumps(report["artifact"], ensure_ascii=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
