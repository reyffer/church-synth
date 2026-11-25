Church Synth for Mac (Audio Unit + VST3)

1. Double-click "Install_Church_Synth.command".
2. A Terminal window installs the plug-ins into your user Library, clears the download
   quarantine, signs them for this Mac and checks the Audio Unit with Apple's validator.
3. Quit MainStage / Logic completely (Cmd-Q) and reopen it.

If macOS says it "could not verify" the script (this happens when the zip was
downloaded with a web browser, because the script isn't signed by Apple):
  - System Settings > Privacy & Security, scroll down to the message about
    Install_Church_Synth.command and click "Open Anyway"; or
  - in Terminal, run:  xattr -dr com.apple.quarantine  and drag this folder onto the
    Terminal window, press Return, then double-click the script again.

It replaces any earlier Church Synth, keeps your presets, favourites and settings
(Documents/Church Synth), and needs no administrator password.
