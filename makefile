superwow_heal_text_disabler.exe: main.c makefile
	cl main.c /Fe:$@ \
		/std:c17 /O2 \
		/link \
		/nologo \
		/NOLOGO \
		/ALIGN:16 /ignore:4108 \
		/DEBUG:NONE \
		/FIXED \
		/MERGE:.rdata=.text \
		/EMITTOOLVERSIONINFO:NO \
		/EMITPOGOPHASEINFO \
		/NODEFAULTLIB \
		/SUBSYSTEM:WINDOWS \
		kernel32.lib \
		shell32.lib \
		user32.lib