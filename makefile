all:
	.\mkDebug.bat
 
release:
	.\mkRelease.bat

clean:
	rd /s /q build\\release