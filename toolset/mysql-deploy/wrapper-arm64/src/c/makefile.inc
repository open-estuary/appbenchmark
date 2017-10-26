# makefile.inc -- Include this file into existing makefile at the very top.

# _VC_MANIFEST_INC specifies whether build is incremental (1 - incremental).
# _VC_MANIFEST_BASENAME specifies name of a temporary resource file.

_VC_BIN_DIR=..\..\bin\\
_VC_LIB_DIR=..\..\lib\\

!if "$(DEBUG)" == "1"
CPPFLAGS=$(CPPFLAGS) /MDd
LFLAGS=$(LFLAGS) /INCREMENTAL
_VC_MANIFEST_INC=1
_VC_MANIFEST_BASENAME=__VC90.Debug

!else
CPPFLAGS=$(CPPFLAGS) /MD
_VC_MANIFEST_INC=0
_VC_MANIFEST_BASENAME=__VC90

!endif

####################################################
# _VC_MANIFEST_EMBED_EXE - command to embed manifest in wrapper.EXE:
# _VC_MANIFEST_EMBED_DLL - command to embed manifest in wrapper.DLL:

!if "$(_VC_MANIFEST_INC)" == "1"

#MT_SPECIAL_RETURN=1090650113
#MT_SPECIAL_SWITCH=-notify_resource_update
MT_SPECIAL_RETURN=0
MT_SPECIAL_SWITCH=
_VC_MANIFEST_EMBED_EXE= \
if exist $@.manifest mt.exe -manifest $@.manifest -out:$(_VC_BIN_DIR)$(_VC_MANIFEST_BASENAME).auto.manifest $(MT_SPECIAL_SWITCH) & \
if "%ERRORLEVEL%" == "$(MT_SPECIAL_RETURN)" \
rc /r $(_VC_BIN_DIR)$(_VC_MANIFEST_BASENAME).auto.rc & \
link $** /out:$(_VC_BIN_DIR)$@ $(LFLAGS)

_VC_MANIFEST_EMBED_DLL= \
if exist $@.manifest mt.exe -manifest $@.manifest -out:$(_VC_LIB_DIR)$(_VC_MANIFEST_BASENAME).auto.manifest $(MT_SPECIAL_SWITCH) & \
if "%ERRORLEVEL%" == "$(MT_SPECIAL_RETURN)" \
rc /r $(_VC_LIB_DIR)$(_VC_MANIFEST_BASENAME).auto.rc & \
link $** /out:$(_VC_LIB_DIR)$@ $(LFLAGS)

!else

_VC_MANIFEST_EMBED_EXE= \
if exist $@.manifest mt.exe -manifest $@.manifest -outputresource:$(_VC_BIN_DIR)$@;1

_VC_MANIFEST_EMBED_DLL= \
if exist $@.manifest mt.exe -manifest $@.manifest -outputresource:$(_VC_LIB_DIR)$@;2

!endif

####################################################
# _VC_MANIFEST_CLEAN - command to clean resources files generated temporarily:

!if "$(_VC_MANIFEST_INC)" == "1"

_VC_MANIFEST_CLEAN=-del $(_VC_BIN_DIR)$(_VC_MANIFEST_BASENAME).auto.res \
    $(_VC_BIN_DIR)$(_VC_MANIFEST_BASENAME).auto.rc \
    $(_VC_BIN_DIR)$(_VC_MANIFEST_BASENAME).auto.manifest \
	$(_VC_LIB_DIR)$(_VC_MANIFEST_BASENAME).auto.res \
    $(_VC_LIB_DIR)$(_VC_MANIFEST_BASENAME).auto.rc \
    $(_VC_LIB_DIR)$(_VC_MANIFEST_BASENAME).auto.manifest

!else

_VC_MANIFEST_CLEAN=

!endif

# End of makefile.inc 
####################################################