# GTA SA iOS 2.2.21 — แพ็ก IPA จาก App Bundle บน Windows

ใช้ Python 3.10 ขึ้นไปและโฟลเดอร์เกมที่ผู้ใช้มีอยู่ สคริปต์ใช้ Original IPA เป็นข้อมูลอ้างอิง และอ่านไฟล์ที่จะใส่ในแพ็กเกจจาก `gtasa.app` โดยตรง

~~~powershell
$root = 'C:\Morgan IOS'
$project = Join-Path $root 'MorganGame'
$report = Join-Path $root ('Builds\Current\GTA_Bundle_' + (Get-Date -Format 'yyyyMMdd_HHmmss') + '.json')
python (Join-Path $project 'Scripts\package-gta-bundle.py') --workspace $root --config (Join-Path $project 'Configuration\gta-bundle-packaging.json') --report $report
~~~

ตรวจ SHA-256 ของ Original IPA จากนั้นเทียบไฟล์ทุกตัวใน bundle กับสมาชิก ZIP เดิม ตรวจ bundle ID, plist, storyboard, assets, Mach-O ARM64 และ dependencies ของ executable และ framework ตรวจขนาด, CRC, SHA-256 และ permission ของผลลัพธ์ทุก ZIP entry โหมด executable ของเกมและสี่ framework ได้ owner execute bit ใน ZIP ส่วนไฟล์ข้อมูลและ weak dylib ที่มาพร้อม baseline ใช้โหมดเดิม

- `REUSED_VERIFIED` ใช้ IPA ใน `Builds\Verified` เมื่อ payload และ ZIP modes ตรงทุก entry และ SHA-256 ของ archive ตรงกับ baseline ที่ยืนยันไว้
- `REUSED_CURRENT` ใช้ผลใน `Builds\Current` ที่ตรวจแล้วเมื่อ Verified IPA ไม่อยู่ในพาธที่กำหนด
- `BUILT_FROM_SOURCE` สร้าง IPA เดียวใน `Builds\Current` เมื่อทั้งสองพาธไม่มี IPA สคริปต์ตรวจพื้นที่และตรวจ ZIP ก่อนเผยแพร่ชื่อไฟล์ปลายทาง
- `FAIL` รายงานไฟล์ที่ขาด, เกิน หรือ hash ผิด และคืน exit code 1 ไฟล์ฐานข้อมูล IDA ทั้งห้าชื่อใน configuration เป็นไฟล์วิเคราะห์นอก baseline และอยู่ในโฟลเดอร์เดิม

## ESign และผลอุปกรณ์

1. ใช้พาธ `artifact.path` ใน JSON ที่สคริปต์รายงาน ย้ายเฉพาะไฟล์ IPA ที่ต้องการทดสอบไปยัง Files บน iPhone แล้วนำเข้าทั้งไฟล์ใน ESign
2. ลงนามด้วย identity และ provisioning ที่ใช้งานได้กับ App ID, อุปกรณ์และ entitlements ของชุดทดสอบ การลงนามเป็นขั้นตอนแยกจาก Packaging และให้ ESign สร้างผลลงนามของตัวเองโดยเก็บ IPA ต้นทางไว้
3. ติดตั้ง เปิดเกม เข้าเมนู และทดสอบเข้า Offline Gameplay บน iPhone 12 บันทึกการลงนาม การติดตั้ง การเปิดแอป และ Offline แยกกัน

ผล Packaging ตรวจ archive เท่านั้น ลายเซ็นเดิมอยู่ใน Mach-O และ CodeResources ตาม baseline ส่วนการตรวจ Apple code signing และ provisioning สำหรับผลที่จะติดตั้งต้องทำจากชุดที่ลงนามจริง [Apple — Provisioning Profiles](https://developer.apple.com/documentation/technotes/tn3125-inside-code-signing-provisioning-profiles)

สำหรับ Git ให้ stage เฉพาะสคริปต์, configuration และเอกสารนี้ รายงาน JSON ของเครื่อง, GTA bundle, IPA และข้อมูลลงนามเก็บแยกจาก checkout
