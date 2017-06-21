## OSX Disk Image (DMG)

### creating a HiDPI background
tiffutil -cathidpicheck mac/background.png mac/background\@2x.png -out mac/background.tiff 

### creating the disk image
./mac/macdeployqtplus DigitalBitbox.app -dmg -fancy mac/fancy.plist -verbose 2 -volname DigitalBitbox
