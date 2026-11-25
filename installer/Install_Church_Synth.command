#!/bin/zsh
# Installs Church Synth (Audio Unit and VST3) for the current user and makes
# MainStage / Logic rescan it. Double-click it (the first time: right-click > Open).
set -e
cd "$(dirname "$0")"

AU="$HOME/Library/Audio/Plug-Ins/Components"
VST3="$HOME/Library/Audio/Plug-Ins/VST3"
mkdir -p "$AU" "$VST3"
rm -rf "$AU/Church Synth.component" "$VST3/Church Synth.vst3"
cp -R "Church Synth.component" "$AU/"
cp -R "Church Synth.vst3" "$VST3/"

# Files downloaded or copied from another Mac are quarantined; clear that and re-sign locally.
for p in "$AU/Church Synth.component" "$VST3/Church Synth.vst3"; do
    xattr -dr com.apple.quarantine "$p" 2>/dev/null || true
    codesign --force --deep -s - "$p"
done

# Make macOS pick up the new Audio Unit.
killall -9 AudioComponentRegistrar 2>/dev/null || true

echo ""
echo "Validating..."
if auval -v aumu Chsy Ryfr | grep -q "AU VALIDATION SUCCEEDED"; then
    echo "Church Synth $(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$AU/Church Synth.component/Contents/Info.plist") is installed and passed validation (Audio Unit and VST3)."
    echo "Quit MainStage completely (Cmd-Q) and reopen it, then find it under AU Instruments > Reyffer > Church Synth."
else
    echo "Installed, but validation did not pass. Run this for details:"
    echo "  auval -v aumu Chsy Ryfr"
fi
