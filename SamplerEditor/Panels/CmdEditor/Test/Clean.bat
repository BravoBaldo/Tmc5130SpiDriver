@echo off
FOR %%I in (
	.
) DO (
	@echo.
	@echo *******************************************
	@echo **** %%I
	@echo *******************************************
	
	rmdir /S /Q %%I\Release
	rmdir /S /Q %%I\Debug
	rmdir /S /Q %%I\vc_msw
	rmdir /S /Q %%I\vc_mswd
	rmdir /S /Q %%I\vc_x64_mswud
	rmdir /S /Q %%I\vc_x64_mswudll
	rmdir /S /Q %%I\vc_x64_mswuddll
	rmdir /S /Q %%I\vc_x64_mswu
	del      /Q %%I\*.ncb
	del      /Q %%I\*.aps
	del      /Q %%I\*.pch
	del      /Q %%I\*.user
	del      /Q %%I\*.log
	del      /Q %%I\*.bak
	del      /Q %%I\Kikko.txt
	del      /Q %%I\Log.csv
	del      /Q %%I\Kikko.txt
	del      /Q %%I\Log.csv
	del   /F /Q %%I\*.suo
	del  /AH    %%I\*.suo
	del  /AH    %%I\Thumbs.db
	del      /Q %%I\*.csv
	rmdir /S /Q %%I\.vs
	del      /Q %%I\*.pdb
	del      /Q %%I\UpgradeLog.htm
	del      /Q %%I\vc141.idb
	del      /Q %%I\*.vcproj.*.user	
	rmdir /S /Q %%I\CodeGraphData	
)

PAUSE