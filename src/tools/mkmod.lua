#!/usr/local/bin/lua
--[[
  mkmod.lua - produces makefiles for various compilers from a single *.build file.
  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
--]]

--[[
  Synopsis: lua mkmod.lua <filename>
	If processing completes successfully, binary modules are placed in ../../bin directory;
	*.lib or *.a files - in  ../../lib directory.
  Note: BUILD_ENV environment variable must be set before.

  Notes for C programmers: 
	1. the first element of each array has index 1.
	2. only nil and false values are false, all other including 0 are true

  When you use Windows SDK don't forget about the following:
	* ntdll.lib are missing - copy them from DDK
	* SSE2 processor is required to run produced binaries
	* kernel mode driver cannot be compiled
--]]

name, deffile, baseaddr, nativedll, umentry = "", "", "", 0, ""
src, rc, libs, adlibs = {}, {}, {}, {}

input_filename = ""
target_type, target_ext, target_name = "", "", ""
arch = ""

-- FIXME: Only DDK compiler can compile native executable 
-- with static zenwinx and udefrag libraries.
static_lib = 0

ddk_cmd = "build.exe"
msvc_cmd = "nmake.exe /NOLOGO /A /f"
mingw_cmd = "mingw32-make --always-make -f Makefile.mingw"
mingw_x64_cmd = "gmake --always-make -f Makefile_x64.mingw"

-- common subroutines
function copy(src, dst)
	if os.execute("cmd.exe /C copy /Y " .. src .. " " .. dst) ~= 0 then
		error("Can't copy from " .. src .. " to " .. dst .. "!");
	end
end

-- frontend subroutines
function obsolete(src, dst)
--[[	my ($src_mtime,$dst_mtime);

	unless(-f $src){
		die("Source not found!");
		return;
	}
	unless(-f $dst){
		return 1;
	}
    $src_mtime = (stat($src))[9];
	$dst_mtime = (stat($dst))[9];
	--print "src:$src_mtime, dst:$dst_mtime\n";
 	return ($dst_mtime > $src_mtime) ? 0 : 1;
--]]
	return true
end

-- WinDDK backend
makefile_contents = [[
#
# DO NOT EDIT THIS FILE!!!  Edit .\sources if you want to add a new source
# file to this component.  This file merely indirects to the real make file
# that is shared by all the driver components of the Windows NT DDK
#

!INCLUDE $(NTMAKEENV)\makefile.def

]]
function produce_ddk_makefile()
	local t, umt

	local f = assert(io.open(".\\makefile","w"))
	f:write(makefile_contents)
	f:close()

	f = assert(io.open(".\\sources","w"))

	f:write("TARGETNAME=", name, "\n")
	f:write("TARGETPATH=obj\n")

	-- f:write("AMD64_OPTIMIZATION=/Od\n")
	-- f:write("IA64_OPTIMIZATION=/Od\n\n")
	-- f:write("386_OPTIMIZATION=/Ot /Og\n") -- never tested!!!

	if     target_type == "console" then t = "PROGRAM"; umt = "console"
	elseif target_type == "gui"     then t = "PROGRAM"; umt = "windows"
	elseif target_type == "native"  then t = "PROGRAM"; umt = "nt"
	elseif target_type == "driver"  then t = "DRIVER"
	elseif target_type == "dll"     then t = "DYNLINK"; umt = "console"
	else   error("Unknown target type: " .. target_type .. "!")
	end
	
	if static_lib == 1 then
		t = "LIBRARY"
	end

	f:write("TARGETTYPE=", t, "\n\n")
	if target_type == "dll" then
		f:write("DLLDEF=", deffile, "\n\n")
	end

	f:write("USER_C_FLAGS=/DUSE_WINDDK\n\n")
	
	if static_lib == 1 then
		f:write("USER_C_FLAGS=\$(USER_C_FLAGS) /DSTATIC_LIB\n\n")
	end
	
	if target_type == "console" or target_type == "gui" then
		f:write("CFLAGS=\$(CFLAGS) /MT\n\n")
	end

	f:write("SOURCES=")
	for i, v in ipairs(src) do f:write(v, " ") end
	for i, v in ipairs(rc) do f:write(v, " ") end
	f:write("\n")
	
	if target_type == "console" or target_type == "gui" then 
		f:write("USE_MSVCRT=1\n")
	end
	if target_type == "native" then
		f:write("USE_NTDLL=1\n\n")
        
        -- workaround for WDK 7
        -- f:write("MINWIN_SDK_LIB_PATH=\$(SDK_LIB_PATH)\n")
        -- f:write("USER_C_FLAGS=\$(USER_C_FLAGS) /QIfist\n")
	end
	if target_type == "dll" then
		if nativedll == 1 then
			f:write("USE_NTDLL=1\n")
		else
			f:write("USE_MSVCRT=1\n")
		end
	end
	f:write("\n")
	
	if target_type == "native" or target_type == "dll" then
		f:write("# very important for nt 4.0 ")
		f:write("(without RtlUnhandledExceptionFilter function)\n")
		f:write("BUFFER_OVERFLOW_CHECKS=0\n\n")
	end

	f:write("LINKLIBS=")
	for i, v in ipairs(libs) do
		if v ~= "msvcrt" then
			f:write("\$(DDK_LIB_PATH)\\", v, ".lib ")
		end
	end
	for i, v in ipairs(adlibs) do f:write(v, ".lib ") end
	f:write("\n\n")
	
	if target_type ~= "driver" then f:write("UMTYPE=", umt, "\n") end
	if target_type ==  "console" or target_type == "gui" then
		f:write("UMENTRY=", umentry, "\n")
	end
	if target_type == "dll" then
		f:write("DLLBASE=", baseaddr, "\nDLLENTRY=DllMain\n")
	end
	f:close()
end

-- MS Visual Studio backend
function produce_msvc_makefile()
	local s, upname
	local cl_flags, rsc_flags, link_flags

	local f = assert(io.open(".\\" .. name .. ".mak","w"))

	--[[
	OUTDIR and INTDIR parameters are replaced with current directory
	f:write("!IF \"\$(OS)\" == \"Windows_NT\"\n")
	f:write("NULL=\n")
	f:write("!ELSE\n")
	f:write("NULL=nul\n")
	f:write("!ENDIF\n\n")
	--]]

	if os.getenv("BUILD_ENV") == "winsdk" then
		cl_flags = "CPP_PROJ=/nologo /W3 /O2 /D \"WIN32\" /D \"NDEBUG\" /D \"_MBCS\" "
	else
		cl_flags = "CPP_PROJ=/nologo /W3 /O2 /D \"WIN32\" /D \"NDEBUG\" /D \"_MBCS\" "
	end
	
	upname = string.upper(name) .. "_EXPORTS"

	if os.getenv("BUILD_ENV") == "winsdk" then
		cl_flags = cl_flags .. "/D \"USE_WINSDK\" /GS- /arch:SSE2 "
	else
		cl_flags = cl_flags .. "/D \"USE_MSVC\" "
	end

	if target_type == "console" then
		cl_flags = cl_flags .. "/D \"_CONSOLE\" "
		s = "console"
	elseif target_type == "gui" then
		cl_flags = cl_flags .. "/D \"_WINDOWS\" "
		s = "windows"
	elseif target_type == "dll" then
		cl_flags = cl_flags .. "/D \"_CONSOLE\" /D \"_USRDLL\" /D \"" .. upname .. "\" "
		s = "console"
	elseif target_type == "driver" then
		cl_flags = cl_flags .. "/I \"\$(ROSINCDIR)\" /I \"\$(ROSINCDIR)\\ddk\" "
		s = "native"
	elseif target_type == "native" then
		s = "native"
	else error("Unknown target type: " .. target_type .. "!")
	end
	
	-- the following check eliminates need of msvcr90.dll when compiles with SDK
	if nativedll == 0 and os.getenv("BUILD_ENV") ~= "winsdk" then
		cl_flags = cl_flags .. "/MD "
	end
	
	if arch == "i386" then
		cl_flags = cl_flags .. " "
	elseif arch == "amd64" then
		cl_flags = cl_flags .. " "
	elseif arch == "ia64" then
		cl_flags = cl_flags .. " "
	end

	f:write("ALL : \"", name, ".", target_ext, "\"\n\n")
	f:write(cl_flags, " /c \n")

	rsc_flags = "RSC_PROJ=/l 0x409 /d \"NDEBUG\" "
	f:write(rsc_flags, " \n")
	
	link_flags = "LINK32_FLAGS="
	for i, v in ipairs(libs) do
		if nativedll ~= 0 or v ~= "msvcrt" then
			link_flags = link_flags .. v .. ".lib "
		end
	end
	for i, v in ipairs(adlibs) do
		link_flags = link_flags .. v .. ".lib "
	end
	if nativedll == 0 and target_type ~= "native" then
		-- DLL for console/gui environment
		link_flags = link_flags .. "/nologo /incremental:no "
	else
		link_flags = link_flags .. "/nologo /incremental:no /nodefaultlib "
	end
	if arch == "i386" then
		link_flags = link_flags .. "/machine:I386 "
	elseif arch == "amd64" then
		link_flags = link_flags .. "/machine:AMD64 "
	elseif arch == "ia64" then
		link_flags = link_flags .. "/machine:IA64 "
	end
	link_flags = link_flags .. "/subsystem:" .. s .. " "
	if target_type == "dll" then
		if nativedll == 0 then
			link_flags = link_flags .. "/dll "
		else
			link_flags = link_flags .. "/entry:\"DllMain\" /dll "
		end
		link_flags = link_flags .. "/def:" .. deffile .. " "
		link_flags = link_flags .. "/implib:" .. name .. ".lib "
	elseif target_type == "native" then
		link_flags = link_flags .. "/entry:\"NtProcessStartup\" "
	elseif target_type == "driver" then
		link_flags = link_flags .. "/base:\"0x10000\" /entry:\"DriverEntry\" "
		link_flags = link_flags .. "/driver /align:32 "
	end
	f:write(link_flags, " /out:\"", name, ".", target_ext, "\" \n\n")
	
	f:write("CPP=cl.exe\nRSC=rc.exe\nLINK32=link.exe\n\n")
	f:write(".c.obj::\n")
	f:write("    \$(CPP) \@<<\n")
	f:write("    \$(CPP_PROJ) \$<\n")
	f:write("<<\n\n")

	f:write("LINK32_OBJS=")
	for i, v in ipairs(src) do
		f:write(string.gsub(v,"%.c","%.obj"), " ")
	end
	for i, v in ipairs(rc) do
		f:write(string.gsub(v,"%.rc","%.res"), " ")
	end
	f:write("\n\n")
	
	if target_type == "dll" then
		f:write("DEF_FILE=", deffile, "\n\n")
		f:write("\"", name, ".", target_ext, "\" : \$(DEF_FILE) \$(LINK32_OBJS)\n")
		f:write("    \$(LINK32) \@<<\n")
		f:write("  \$(LINK32_FLAGS) \$(LINK32_OBJS)\n")
		f:write("<<\n\n")
	else
		f:write("\"", name, ".", target_ext, "\" : \$(LINK32_OBJS)\n")
		f:write("    \$(LINK32) \@<<\n")
		f:write("  \$(LINK32_FLAGS) \$(LINK32_OBJS)\n")
		f:write("<<\n\n")
	end

	for i, v in ipairs(rc) do
		f:write("SOURCE=", v, "\n\n")
		f:write(string.gsub(v,"%.rc","%.res"), " : \$(SOURCE)\n")
		f:write("    \$(RSC) \$(RSC_PROJ) \$(SOURCE)\n\n")
	end

	f:close()
end

-- MinGW backend
main_mingw_rules = [[
define build_target
@echo Linking...
@$(CC) -o $(TARGET) $(SRC_OBJS) $(RSRC_OBJS) $(LIB_DIRS) $(LIBS) $(LDFLAGS)
endef

define compile_resource
@echo Compiling $<
@$(WINDRES) $(RCFLAGS) $(RC_PREPROC) $(RC_INCLUDE_DIRS) -O COFF -i "$<" -o "$@"
endef

define compile_source
@echo Compiling $<
@$(CC) $(CFLAGS) $(C_PREPROC) $(C_INCLUDE_DIRS) -c "$<" -o "$@"
endef

]]

static_lib_mingw_rules = [[
define build_target
@echo Linking...
@ar rc lib$(TARGET).a $(SRC_OBJS)
@ranlib lib$(TARGET).a
endef

define compile_resource
@echo Compiling $<
@$(WINDRES) $(RCFLAGS) $(RC_PREPROC) $(RC_INCLUDE_DIRS) -O COFF -i "$<" -o "$@"
endef

define compile_source
@echo Compiling $<
@$(CC) $(CFLAGS) $(C_PREPROC) $(C_INCLUDE_DIRS) -c "$<" -o "$@"
endef

]]

function produce_mingw_makefile()
	local adlibs_libs = {}
	local adlibs_paths = {}
	local pos, j

	local f = assert(io.open(".\\Makefile.mingw","w"))

	f:write("PROJECT = ", name, "\nCC = gcc.exe\n\n")
	f:write("WINDRES = \"\$(COMPILER_BIN)windres.exe\"\n\n")
	
	f:write("TARGET = ", target_name, "\n")
	
	f:write("CFLAGS = -pipe  -Wall -g0 -O2")
--	if static_lib == 1 then
--		f:write(" -DSTATIC_LIB")
--	end
	f:write("\n")

	f:write("RCFLAGS = ")
	f:write("\n")
	
	f:write("C_INCLUDE_DIRS = \n")
	f:write("C_PREPROC = \n")
	f:write("RC_INCLUDE_DIRS = \n")
	f:write("RC_PREPROC = \n")
	
	if target_type == "console" then
		f:write("LDFLAGS = -pipe -Wl,--strip-all\n")
	elseif target_type == "gui" then
		f:write("LDFLAGS = -pipe -mwindows -Wl,--strip-all\n")
	elseif target_type == "native" then
		f:write("LDFLAGS = -pipe -nostartfiles -nodefaultlibs ")
		f:write("-Wl,--entry,_NtProcessStartup\@4,--subsystem,native,--strip-all\n")
	elseif target_type == "driver" then
		f:write("LDFLAGS = -pipe -nostartfiles -nodefaultlibs ")
		f:write(name .. "-mingw.def -Wl,--entry,_DriverEntry\@8,")
		f:write("--subsystem,native,--image-base,0x10000,-shared,--strip-all\n")
	elseif target_type == "dll" then
		f:write("LDFLAGS = -pipe -shared -Wl,")
		f:write("--out-implib,lib", name, ".dll.a -nostartfiles ")
		f:write("-nodefaultlibs ", name, "-mingw.def -Wl,--kill-at,")
		f:write("--entry,_DllMain\@12,--strip-all\n")
	else error("Unknown target type: " .. target_type .. "!")
	end

	f:write("LIBS = ")
	for i, v in ipairs(libs) do
		f:write("-l", v, " ")
	end

	j = 1
	for i, v in ipairs(adlibs) do
		pos = 0
		repeat
			pos = string.find(v,"\\",pos + 1,true)
			--FIXME: pos == nil ??? it's unusual, but ...
		until string.find(v,"\\",pos + 1,true) == nil
		adlibs_libs[j] = string.sub(v,pos + 1)
		adlibs_paths[j] = string.sub(v,0,pos - 1)
		j = j + 1
	end
	for i, v in ipairs(adlibs_libs) do
		f:write("-l", v, " ")
	end
	f:write("\nLIB_DIRS = ")
	for i, v in ipairs(adlibs_paths) do
		f:write("-L\"", v, "\" ")
	end
	f:write("\n\n")
	
	f:write("SRC_OBJS = ")
	for i, v in ipairs(src) do
		f:write(string.gsub(v,"%.c","%.o"), " ")
	end

	f:write("\n\nRSRC_OBJS = ")
	for i, v in ipairs(rc) do
		f:write(string.gsub(v,"%.rc","%.res"), " ")
	end
	f:write("\n\n")

	--if static_lib == 0 then
		f:write(main_mingw_rules)
	--else
	--	f:write(static_lib_mingw_rules)
	--end
	
	f:write(".PHONY: print_header\n\n")
	f:write("\$(TARGET): print_header \$(RSRC_OBJS) \$(SRC_OBJS)\n")
	f:write("\t\$(build_target)\n")

	if target_type == "dll" then
		f:write("\t\$(correct_lib)\n")
	end
	
	f:write("\nprint_header:\n")
	f:write("\t\@echo ----------Configuration: ", name, " - Release----------\n\n")
	
	if target_type == "dll" then
		f:write("define correct_lib\n")
		f:write("\t\@echo ------ correct the lib\$(PROJECT).dll.a library ------\n")
		f:write("\t\@dlltool -k --output-lib lib\$(PROJECT).dll.a --def ")
		f:write(name, "-mingw.def\n")
		f:write("endef\n\n")
	end
	
	for i, v in ipairs(src) do
		f:write(string.gsub(v,"%.c","%.o"), ": ")
		f:write(v, "\n\t\$(compile_source)\n\n")
	end

	for i, v in ipairs(rc) do
		f:write(string.gsub(v,"%.rc","%.res"), ": ")
		f:write(v, "\n\t\$(compile_resource)\n\n")
	end

	f:close()
end

-- MinGW x64 backend
function produce_mingw_x64_makefile()
	local adlibs_libs = {}
	local adlibs_paths = {}
	local pos, j

	local f = assert(io.open(".\\Makefile_x64.mingw","w"))
	
	f:write("PROJECT = ", name, "\nCC = x86_64-w64-mingw32-gcc.exe\n\n")
	f:write("WINDRES = \"\$(COMPILER_BIN)x86_64-w64-mingw32-windres.exe\"\n\n")
	
	f:write("TARGET = ", target_name, "\n")
	
	f:write("CFLAGS = -pipe  -Wall -g0 -O2 -m64\n")
	f:write("RCFLAGS = \n")
	
	f:write("C_INCLUDE_DIRS = \n")
	f:write("C_PREPROC = \n")
	f:write("RC_INCLUDE_DIRS = \n")
	f:write("RC_PREPROC = \n")
	
	if target_type == "console" then
		f:write("LDFLAGS = -pipe -Wl,--strip-all\n")
	elseif target_type == "gui" then
		f:write("LDFLAGS = -pipe -mwindows -Wl,--strip-all\n")
	elseif target_type == "native" then
		f:write("LDFLAGS = -pipe -nostartfiles -nodefaultlibs ")
		f:write("-Wl,--entry,_NtProcessStartup\@4,--subsystem,native,--strip-all\n")
	elseif target_type == "driver" then
		f:write("LDFLAGS = -pipe -nostartfiles -nodefaultlibs ")
		f:write(name .. "-mingw.def -Wl,--entry,_DriverEntry\@8,")
		f:write("--subsystem,native,--image-base,0x10000,-shared,--strip-all\n")
	elseif target_type == "dll" then
		f:write("LDFLAGS = -pipe -shared -Wl,")
		f:write("--out-implib,lib", name, ".dll.a -nostartfiles ")
		if nativedll == 0 then
			f:write(name, "-mingw.def -Wl,--kill-at,")
		else
			f:write("-nodefaultlibs ", name, "-mingw.def -Wl,--kill-at,")
		end
		f:write("--entry,_DllMain\@12,--strip-all\n")
	else error("Unknown target type: " .. target_type .. "!")
	end

	f:write("LIBS = ")
	for i, v in ipairs(libs) do
		f:write("-l", v, " ")
	end

	j = 1
	for i, v in ipairs(adlibs) do
		pos = 0
		repeat
			pos = string.find(v,"\\",pos + 1,true)
			--FIXME: pos == nil ??? it's unusual, but ...
		until string.find(v,"\\",pos + 1,true) == nil
		adlibs_libs[j] = string.sub(v,pos + 1)
		adlibs_paths[j] = string.sub(v,0,pos - 1)
		j = j + 1
	end
	for i, v in ipairs(adlibs_libs) do
		f:write("-l", v, " ")
	end
	f:write("\nLIB_DIRS = ")
	for i, v in ipairs(adlibs_paths) do
		f:write("-L\"", v, "\" ")
	end
	f:write("\n\n")
	
	f:write("SRC_OBJS = ")
	for i, v in ipairs(src) do
		f:write(string.gsub(v,"%.c","%.o"), " ")
	end

	f:write("\n\nRSRC_OBJS = ")
	for i, v in ipairs(rc) do
		f:write(string.gsub(v,"%.rc","%.res"), " ")
	end
	f:write("\n\n")

	f:write(main_mingw_rules)
	
	f:write(".PHONY: print_header\n\n")
	f:write("\$(TARGET): print_header \$(RSRC_OBJS) \$(SRC_OBJS)\n")
	f:write("\t\$(build_target)\n")

	if target_type == "dll" then
		f:write("\t\$(correct_lib)\n")
	end
	
	f:write("\nprint_header:\n")
	f:write("\t\@echo ----------Configuration: ", name, " - Release----------\n\n")
	
	if target_type == "dll" then
		f:write("define correct_lib\n")
		f:write("\t\@echo ------ correct the lib\$(PROJECT).dll.a library ------\n")
		f:write("\t\@x86_64-w64-mingw32-dlltool -k --output-lib lib\$(PROJECT).dll.a --def ")
		f:write(name, ".def\n")
		f:write("endef\n\n")
	end
	
	for i, v in ipairs(src) do
		f:write(string.gsub(v,"%.c","%.o"), ": ")
		f:write(v, "\n\t\$(compile_source)\n\n")
	end

	for i, v in ipairs(rc) do
		f:write(string.gsub(v,"%.rc","%.res"), ": ")
		f:write(v, "\n\t\$(compile_resource)\n\n")
	end

	f:close()
end

-- frontend
input_filename = arg[1]
if input_filename == nil then
	error("Filename must be specified!")
end
print(input_filename .. " Preparing the makefile generation...\n")

dofile(input_filename)

if arg[2] ~= nil then
	if arg[2] == "static-lib" then
		static_lib = 1
	end
end

if target_type == "console" or target_type == "gui" or target_type == "native" then
	target_ext = "exe"
elseif target_type == "dll" then
	target_ext = "dll"
elseif target_type == "driver" then
	target_ext = "sys"
else
	error("Unknown target type: " .. target_type .. "!")
end
target_name = name .. "." .. target_ext

if os.getenv("BUILD_ENV") == "winddk" then
	if obsolete(input_filename,".\\sources") then
		produce_ddk_makefile()
	end
	print(input_filename .. " winddk build performing...\n")
	arch = "i386"
	if os.getenv("AMD64") ~= nil then arch = "amd64" end
	if os.getenv("IA64") ~= nil then arch = "ia64" end
	ddk_cmd = ddk_cmd .. " -c"
	if os.execute(ddk_cmd) ~= 0 then
		error("Can't build the target!")
	end
	if static_lib == 0 then
		if arch == "i386" then
			copy("objfre_wnet_x86\\i386\\" .. target_name,"..\\..\\bin\\")
		else
			copy("objfre_wnet_" .. arch .. "\\" .. arch .. "\\" .. target_name,
				"..\\..\\bin\\" .. arch .. "\\")
		end
	end
	if target_type == "dll" then
		if arch == "i386" then
			copy("objfre_wnet_x86\\i386\\" .. name .. ".lib","..\\..\\lib\\")
		else
			copy("objfre_wnet_" .. arch .. "\\" .. arch .. "\\" .. name .. ".lib",
				 "..\\..\\lib\\" .. arch .. "\\" .. name .. ".lib")
		end
	end
elseif os.getenv("BUILD_ENV") == "winsdk" then
	if target_type == "driver" then
		print("Driver compilation is not supported by Windows SDK.\n")
	else
		if obsolete(input_filename, name .. ".mak") then
			produce_msvc_makefile()
		end
		print(input_filename .. " windows sdk build performing...\n")
		arch = "i386"
		if os.getenv("AMD64") ~= nil then arch = "amd64" end
		if os.getenv("IA64") ~= nil then arch = "ia64" end
		msvc_cmd = msvc_cmd .. name .. ".mak"
		if os.execute(msvc_cmd) ~= 0 then
			error("Can't build the target!")
		end
		if static_lib == 0 then
			if arch == "i386" then
				copy(target_name, "..\\..\\bin\\")
			else
				copy(target_name, "..\\..\\bin\\" .. arch .. "\\")
			end
		end
		if target_type == "dll" then
			if arch == "i386" then
				copy(name .. ".lib", "..\\..\\lib\\")
			else
				copy(name .. ".lib", "..\\..\\lib\\" .. arch .. "\\")
			end
		end
	end
elseif os.getenv("BUILD_ENV") == "msvc" then
	if obsolete(input_filename, name .. ".mak") then
		produce_msvc_makefile()
	end
	print(input_filename .. " msvc build performing...\n")
	msvc_cmd = msvc_cmd .. name .. ".mak"
	if os.execute(msvc_cmd) ~= 0 then
		error("Can't build the target!")
	end
	if static_lib == 0 then
		copy(target_name,"..\\..\\bin\\")
	end
	if target_type == "dll" then
		copy(name .. ".lib","..\\..\\lib\\")
	end
elseif os.getenv("BUILD_ENV") == "mingw" then
	if obsolete(input_filename, ".\\Makefile.mingw") then
		produce_mingw_makefile()
	end
	print(input_filename .. " mingw build performing...\n")
	if os.execute(mingw_cmd) ~= 0 then
		error("Can't build the target!")
	end
	if static_lib == 0 then
		copy(target_name,"..\\..\\bin\\")
	end
	if target_type == "dll" then
		copy("lib" .. target_name .. ".a","..\\..\\lib\\")
	end
elseif os.getenv("BUILD_ENV") == "mingw_x64" then
	-- NOTE: MinGW x64 compiler currently generates wrong code, therefore we cannot use it for real purposes.
	if target_type == "driver" then
		print("Driver compilation is not supported by x64 MinGW.\n")
	else
		if obsolete(input_filename, ".\\Makefile_x64.mingw") then
			produce_mingw_x64_makefile()
		end
		print(input_filename .. " mingw build performing...\n")
		if os.execute(mingw_x64_cmd) ~= 0 then
			error("Can't build the target!")
		end
		if static_lib == 0 then
			copy(target_name,"..\\..\\bin\\amd64\\")
		end
		if target_type == "dll" then
			copy("lib" .. target_name .. ".a","..\\..\\lib\\amd64\\")
		end
	end
else
	error("\%BUILD_ENV\% has wrong value: " .. os.getenv("BUILD_ENV") .. "!")
end

print(input_filename .. " " .. os.getenv("BUILD_ENV") .. " build was successful.\n")
