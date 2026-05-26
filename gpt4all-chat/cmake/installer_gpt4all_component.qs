function Component() {
}

var targetDirectory;
Component.prototype.beginInstallation = function() {
    targetDirectory = installer.value("TargetDir");
};

Component.prototype.createOperations = function() {
    try {
        // call the base create operations function
        component.createOperations();
        if (systemInfo.productType === "windows") {
            try {
                var userProfile = installer.environmentVariable("USERPROFILE");
                installer.setValue("UserProfile", userProfile);
                component.addOperation("CreateShortcut",
                    targetDirectory + "/bin/chat.exe",
                    "@UserProfile@/Desktop/TAi Studio.lnk",
                    "workingDirectory=" + targetDirectory + "/bin",
                    "iconPath=" + targetDirectory + "/gpt4all.ico",
                    "iconId=0", "description=Open TAi Studio");
            } catch (e) {
                print("ERROR: creating desktop shortcut" + e);
            }
            component.addOperation("CreateShortcut",
                targetDirectory + "/bin/chat.exe",
                "@StartMenuDir@/TAi Studio.lnk",
                "workingDirectory=" + targetDirectory + "/bin",
                "iconPath=" + targetDirectory + "/gpt4all.ico",
                "iconId=0", "description=Open TAi Studio");
        } else if (systemInfo.productType === "macos") {
            var gpt4allAppPath = targetDirectory + "/bin/gpt4all.app";
            var symlinkPath = targetDirectory + "/../TAi Studio.app";
            // Remove the symlink if it already exists
            component.addOperation("Execute", "rm", "-f", symlinkPath);
            // Create the symlink
            component.addOperation("Execute", "ln", "-s", gpt4allAppPath, symlinkPath);
        } else { // linux
            var homeDir = installer.environmentVariable("HOME");
            if (!installer.fileExists(homeDir + "/Desktop/TAi Studio.desktop")) {
                component.addOperation("CreateDesktopEntry",
                    homeDir + "/Desktop/TAi Studio.desktop",
                    "Type=Application\nTerminal=false\nExec=\"" + targetDirectory +
                    "/bin/chat\"\nName=TAi Studio\nIcon=" + targetDirectory +
                    "/gpt4all-48.png\nName[en_US]=TAi Studio");
            }
        }
    } catch (e) {
        print("ERROR: running post installscript.qs" + e);
    }
}

Component.prototype.createOperationsForArchive = function(archive)
{
    component.createOperationsForArchive(archive);

    if (systemInfo.productType === "macos") {
        var uninstallTargetDirectory = installer.value("TargetDir");
        var symlinkPath = uninstallTargetDirectory + "/../TAi Studio.app";

        // Remove the symlink during uninstallation
        if (installer.isUninstaller()) {
            component.addOperation("Execute", "rm", "-f", symlinkPath, "UNDOEXECUTE");
        }
    }
}
