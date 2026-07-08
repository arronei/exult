@if exist usecode.uc (
	@..\..\..\ucc.exe -o usecode %0 > NUL: 2>&1
	@if errorlevel 9009 (
		@echo ucc.exe was not found at the repo root; build it first via
		@echo   make -f Makefile.mingw ucc.exe
		@echo.
		@exit
	)

	@echo Compiling Usecode...
	@..\..\..\ucc.exe -o usecode usecode.uc
	@if errorlevel 1 (
		@echo There were error^(s^) compiling usecode!
		@pause
	) else (
		@echo Usecode has been successfuly compiled!
	)
)
