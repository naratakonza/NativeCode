#import "MGStartupKeychain.h"

#import <Foundation/Foundation.h>
#import <Security/Security.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mach/mach.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>

static NSString *MGAppKeychainGroup;
static NSString *MGRockstarKeychainGroup;
static std::atomic<unsigned> MGStartupKeychainCalls{0};

static OSStatus MGLogKeychainStatus(const char *operation, bool redirected, OSStatus status)
{
    if (MGStartupKeychainCalls.fetch_add(1, std::memory_order_relaxed) < 16) {
        NSLog(@"[MorganLauncher] Keychain compatibility: %s %s OSStatus=%d",
            operation, redirected ? "mapped" : "original", status);
    }
    return status;
}

static NSString *MGProbeKeychainGroup(NSString *group, OSStatus *status)
{
    NSMutableDictionary *item = [@{
        (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
        (__bridge id)kSecAttrService: @"com.morgangame.ios.keychain-probe",
        (__bridge id)kSecAttrAccount: NSUUID.UUID.UUIDString
    } mutableCopy];
    if (group) item[(__bridge id)kSecAttrAccessGroup] = group;

    NSMutableDictionary *add = [item mutableCopy];
    add[(__bridge id)kSecValueData] = [NSData dataWithBytes:"\0" length:1];
    add[(__bridge id)kSecReturnAttributes] = @YES;

    CFTypeRef result = nullptr;
    *status = SecItemAdd((__bridge CFDictionaryRef)add, &result);
    id attributes = CFBridgingRelease(result);
    if (*status != errSecSuccess) return nil;

    NSString *actualGroup = nil;
    if ([attributes isKindOfClass:NSDictionary.class]) {
        id value = attributes[(__bridge id)kSecAttrAccessGroup];
        if ([value isKindOfClass:NSString.class]) actualGroup = value;
    }
    if (actualGroup) item[(__bridge id)kSecAttrAccessGroup] = actualGroup;
    *status = SecItemDelete((__bridge CFDictionaryRef)item);
    return *status == errSecSuccess ? actualGroup : nil;
}

static NSDictionary *MGRockstarKeychainRequest(CFDictionaryRef request, bool &redirected)
{
    NSDictionary *query = (__bridge NSDictionary *)request;
    if (![query[(__bridge id)kSecClass] isEqual:(__bridge id)kSecClassGenericPassword]) return query;

    id group = query[(__bridge id)kSecAttrAccessGroup];
    const bool isSeedProbe = !group
        && [query[(__bridge id)kSecAttrAccount] isEqual:@"bundleSeedID"]
        && [query[(__bridge id)kSecAttrService] isEqual:@""];
    if (!isSeedProbe && ![group isEqual:MGRockstarKeychainGroup]) return query;

    NSMutableDictionary *adjusted = [query mutableCopy];
    adjusted[(__bridge id)kSecAttrAccessGroup] = MGAppKeychainGroup;
    redirected = true;
    return adjusted;
}

static OSStatus MGRockstarSecItemAdd(CFDictionaryRef attributes, CFTypeRef *result)
{
    bool redirected = false;
    NSDictionary *request = MGRockstarKeychainRequest(attributes, redirected);
    const OSStatus status = SecItemAdd((__bridge CFDictionaryRef)request, result);
    return MGLogKeychainStatus("SecItemAdd", redirected, status);
}

static OSStatus MGRockstarSecItemCopyMatching(CFDictionaryRef query, CFTypeRef *result)
{
    bool redirected = false;
    NSDictionary *request = MGRockstarKeychainRequest(query, redirected);
    const OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)request, result);
    return MGLogKeychainStatus("SecItemCopyMatching", redirected, status);
}

static OSStatus MGRockstarSecItemDelete(CFDictionaryRef query)
{
    bool redirected = false;
    NSDictionary *request = MGRockstarKeychainRequest(query, redirected);
    const OSStatus status = SecItemDelete((__bridge CFDictionaryRef)request);
    return MGLogKeychainStatus("SecItemDelete", redirected, status);
}

static OSStatus MGRockstarSecItemUpdate(CFDictionaryRef query, CFDictionaryRef attributes)
{
    bool redirected = false;
    NSDictionary *request = MGRockstarKeychainRequest(query, redirected);
    const OSStatus status = SecItemUpdate((__bridge CFDictionaryRef)request, attributes);
    return MGLogKeychainStatus("SecItemUpdate", redirected, status);
}

static const mach_header_64 *MGFindRockstarMobile()
{
    constexpr char suffix[] = "/RockstarMobile.framework/RockstarMobile";
    for (uint32_t i = 0; i < _dyld_image_count(); ++i) {
        const char *name = _dyld_get_image_name(i);
        if (!name) continue;
        const auto length = std::strlen(name);
        if (length >= sizeof(suffix) - 1 && std::strcmp(name + length - (sizeof(suffix) - 1), suffix) == 0) {
            return reinterpret_cast<const mach_header_64 *>(_dyld_get_image_header(i));
        }
    }
    return nullptr;
}

static uintptr_t *MGFindRockstarKeychainSlots()
{
    constexpr std::array<uint8_t, 16> expectedUUID{
        0x5F, 0xA6, 0x58, 0x7E, 0xF7, 0x2D, 0x34, 0xF2,
        0xAC, 0x8E, 0x80, 0x3E, 0x77, 0x9B, 0x3B, 0xD6
    };
    // Four adjacent ARM64 lazy pointers: SecItemAdd, CopyMatching, Delete, Update.
    constexpr uint64_t firstSlot = 0x321878;
    constexpr uint64_t slotBytes = 4 * sizeof(uintptr_t);

    const auto *image = MGFindRockstarMobile();
    if (!image || image->magic != MH_MAGIC_64 || image->cputype != CPU_TYPE_ARM64
        || (image->cpusubtype & ~CPU_SUBTYPE_MASK) != CPU_SUBTYPE_ARM64_ALL
        || image->filetype != MH_DYLIB || image->sizeofcmds > 0x4000) return nullptr;

    bool hasUUID = false;
    bool hasSlots = false;
    auto cursor = reinterpret_cast<const uint8_t *>(image + 1);
    auto remaining = image->sizeofcmds;
    for (uint32_t i = 0; i < image->ncmds; ++i) {
        if (remaining < sizeof(load_command)) return nullptr;
        const auto *command = reinterpret_cast<const load_command *>(cursor);
        if (command->cmdsize < sizeof(load_command) || command->cmdsize % 8 != 0
            || command->cmdsize > remaining) return nullptr;

        if (command->cmd == LC_UUID) {
            if (hasUUID || command->cmdsize != sizeof(uuid_command)) return nullptr;
            const auto *uuid = reinterpret_cast<const uuid_command *>(cursor);
            hasUUID = std::memcmp(uuid->uuid, expectedUUID.data(), expectedUUID.size()) == 0;
            if (!hasUUID) return nullptr;
        } else if (command->cmd == LC_SEGMENT_64) {
            if (command->cmdsize < sizeof(segment_command_64)) return nullptr;
            const auto *segment = reinterpret_cast<const segment_command_64 *>(cursor);
            if (segment->nsects > (command->cmdsize - sizeof(*segment)) / sizeof(section_64)) return nullptr;
            if (std::strncmp(segment->segname, "__DATA", sizeof(segment->segname)) == 0 && (segment->initprot & VM_PROT_WRITE)) {
                const auto *section = reinterpret_cast<const section_64 *>(segment + 1);
                for (uint32_t j = 0; j < segment->nsects; ++j, ++section) {
                    if ((section->flags & SECTION_TYPE) == S_LAZY_SYMBOL_POINTERS
                        && std::strncmp(section->sectname, "__la_symbol_ptr", sizeof(section->sectname)) == 0
                        && firstSlot >= section->addr && section->size >= slotBytes
                        && firstSlot - section->addr <= section->size - slotBytes) hasSlots = true;
                }
            }
        }
        remaining -= command->cmdsize;
        cursor += command->cmdsize;
    }
    if (!hasUUID || !hasSlots || remaining != 0) return nullptr;

    const vm_address_t address = reinterpret_cast<vm_address_t>(image) + firstSlot;
    vm_address_t region = address;
    vm_size_t length = 0;
    vm_region_basic_info_data_64_t info{};
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object = MACH_PORT_NULL;
    const kern_return_t status = vm_region_64(mach_task_self(), &region, &length, VM_REGION_BASIC_INFO_64,
        reinterpret_cast<vm_region_info_t>(&info), &count, &object);
    if (MACH_PORT_VALID(object)) mach_port_deallocate(mach_task_self(), object);
    if (status != KERN_SUCCESS || !(info.protection & VM_PROT_WRITE) || region > address
        || length < slotBytes || address - region > length - slotBytes) {
        NSLog(@"[MorganLauncher] Keychain compatibility: slot memory check failed (Mach=%d, protection=%d)",
            status, info.protection);
        return nullptr;
    }
    return reinterpret_cast<uintptr_t *>(address);
}

void MGInstallStartupKeychain()
{
    auto *slots = MGFindRockstarKeychainSlots();
    if (!slots) {
        NSLog(@"[MorganLauncher] Keychain compatibility: Rockstar import slots unavailable");
        return;
    }

    OSStatus status = errSecSuccess;
    NSString *defaultGroup = MGProbeKeychainGroup(nil, &status);
    NSString *bundleID = NSBundle.mainBundle.bundleIdentifier;
    const NSRange separator = [defaultGroup rangeOfString:@"."];
    if (!defaultGroup || separator.location == NSNotFound || separator.location == 0
        || ![bundleID isEqualToString:@"com.morgangame.ios"]) {
        NSLog(@"[MorganLauncher] Keychain compatibility: default group unavailable (%d)", status);
        return;
    }

    NSString *prefix = [defaultGroup substringToIndex:separator.location];
    NSString *rockstarGroup = [prefix stringByAppendingString:@".rockstar"];
    if ([defaultGroup isEqualToString:rockstarGroup]
        || [MGProbeKeychainGroup(rockstarGroup, &status) isEqualToString:rockstarGroup]) {
        NSLog(@"[MorganLauncher] Keychain compatibility: SDK group available");
        return;
    }

    NSLog(@"[MorganLauncher] Keychain compatibility: SDK group probe OSStatus=%d", status);
    // Security returned this group from a successful add/delete under the current signature.
    MGAppKeychainGroup = defaultGroup;
    MGRockstarKeychainGroup = rockstarGroup;
    const std::array<uintptr_t, 4> replacements{
        reinterpret_cast<uintptr_t>(&MGRockstarSecItemAdd),
        reinterpret_cast<uintptr_t>(&MGRockstarSecItemCopyMatching),
        reinterpret_cast<uintptr_t>(&MGRockstarSecItemDelete),
        reinterpret_cast<uintptr_t>(&MGRockstarSecItemUpdate)
    };
    for (std::size_t i = 0; i < replacements.size(); ++i) {
        __atomic_store_n(slots + i, replacements[i], __ATOMIC_RELEASE);
    }
    unsigned installed = 0;
    for (std::size_t i = 0; i < replacements.size(); ++i) {
        if (__atomic_load_n(slots + i, __ATOMIC_ACQUIRE) == replacements[i]) ++installed;
    }
    NSLog(@"[MorganLauncher] Keychain compatibility: Rockstar default group, slots verified=%u/4", installed);
}
