--
-- Name:        premake-ninja/ninja.lua
-- Purpose:     Define the ninja action.
-- Author:      Dmitry Ivanov
-- Created:     2015/07/04
-- Copyright:   (c) 2015 Dmitry Ivanov
--

local p = premake
local tree = p.tree
local project = p.project
local solution = p.solution
local config = p.config
local fileconfig = p.fileconfig

premake.modules.ninja = {}
local ninja = p.modules.ninja

function ninja.esc(value)
	value = value:gsub("%$", "$$") -- TODO maybe there is better way
	value = value:gsub(":", "$:")
	value = value:gsub("\n", "$\n")
	value = value:gsub(" ", "$ ")
	return value
end

-- in some cases we write file names in rule commands directly
-- so we need to propely escape them
function ninja.shesc(value)
	if type(value) == "table" then
		local result = {}
		local n = #value
		for i = 1, n do
			table.insert(result, ninja.shesc(value[i]))
		end
		return result
	end

	if value:find(" ") then
		return "\"" .. value .. "\""
	end

	return value
end

-- generate solution that will call ninja for projects
function ninja.generateSolution(sln)
	p.w("# solution build file")
	p.w("# generated with premake ninja")
	p.w("")

	p.w("# build projects")
	local cfgs = {} -- key is concatanated name or variant name, value is string of outputs names
	local key = ""
	local cfg_first = nil
	local cfg_first_lib = nil

	for prj in solution.eachproject(sln) do
		for cfg in project.eachconfig(prj) do
			key = prj.name .. "_" .. cfg.buildcfg

			if cfg.platform ~= nil then key = key .. "_" .. cfg.platform end

			-- fill list of output files
			if not cfgs[key] then cfgs[key] = "" end
			cfgs[key] = p.esc(ninja.outputFilename(cfg)) .. " "

			if not cfgs[cfg.buildcfg] then cfgs[cfg.buildcfg] = "" end
			cfgs[cfg.buildcfg] = cfgs[cfg.buildcfg] .. p.esc(ninja.outputFilename(cfg)) .. " "

			-- set first configuration name
			if (cfg_first == nil) and (cfg.kind == p.CONSOLEAPP or cfg.kind == p.WINDOWEDAPP) then
				cfg_first = key
			end
			if (cfg_first_lib == nil) and (cfg.kind == p.STATICLIB or cfg.kind == p.SHAREDLIB) then
				cfg_first_lib = key
			end

			-- include other ninja file
			p.w("subninja " .. p.esc(ninja.projectCfgFilename(cfg, true)))
		end
	end

	if cfg_first == nil then cfg_first = cfg_first_lib end

	p.w("")

	p.w("# targets")
	for cfg, outputs in pairs(cfgs) do
		p.w("build " .. p.esc(cfg) .. ": phony " .. outputs)
	end
	p.w("")

	p.w("# default target")
	p.w("default " .. p.esc(cfg_first))
	p.w("")
end

function ninja.list(value)
	if #value > 0 then
		return " " .. table.concat(value, " ")
	else
		return ""
	end
end

-- generate project + config build file
function ninja.generateProjectCfg(cfg)
	local toolset_name = _OPTIONS.cc or cfg.toolset
	local system_name = os.get()

	if toolset_name == nil then -- TODO why premake doesn't provide default name always ?
		if system_name == "windows" then
			toolset_name = "msc"
		elseif system_name == "macosx" then
			toolset_name = "clang"
		elseif system_name == "linux" then
			toolset_name = "gcc"
		else
			toolset_name = "gcc"
			p.warnOnce("unknown_system", "no toolchain set and unknown system " .. system_name .. " so assuming toolchain is gcc")
		end
	end

	local prj = cfg.project
	local toolset = p.tools[toolset_name]

	p.w("# project build file")
	p.w("# generated with premake ninja")
	p.w("")

	-- premake-ninja relies on scoped rules
	-- and they were added in ninja v1.6
	p.w("ninja_required_version = 1.6")
	p.w("")

	---------------------------------------------------- figure out toolset executables
	local cc = ""
	local cxx = ""
	local ar = ""
	local link = ""
	
	if toolset_name == "msc" then
		-- TODO premake doesn't set tools names for msc, do we want to fix it ?
		cc = "cl"
		cxx = "cl"
		ar = "lib"
		link = "cl"
	elseif toolset_name == "clang" then
		cc = toolset:gettoolname("cc")
		cxx = toolset:gettoolname("cxx")
		ar = toolset:gettoolname("ar")
		link = toolset:gettoolname(iif(cfg.language == "C", "cc", "cxx"))
	elseif toolset_name == "gcc" then
		if not cfg.gccprefix then cfg.gccprefix = "" end
		cc = toolset.gettoolname(cfg, "cc")
		cxx = toolset.gettoolname(cfg, "cxx")
		ar = toolset.gettoolname(cfg, "ar")
		link = toolset.gettoolname(cfg, iif(cfg.language == "C", "cc", "cxx"))
	else
		p.error("unknown toolchain " .. toolset_name)
	end

	---------------------------------------------------- figure out settings
	globalincludes = {}
	table.foreachi(cfg.includedirs, function(v)
		-- TODO this is a bit obscure and currently I have no idea why exactly it's working
		globalincludes[#globalincludes + 1] = project.getrelative(cfg.workspace, v)
	end)

	local buildopt =		ninja.list(cfg.buildoptions)
	local cflags =			ninja.list(toolset.getcflags(cfg))
	local cppflags =		ninja.list(toolset.getcppflags(cfg))
	local cxxflags =		ninja.list(toolset.getcxxflags(cfg))
	local warnings =		""
	local defines =			ninja.list(table.join(toolset.getdefines(cfg.defines), toolset.getundefines(cfg.undefines)))
	local includes =		ninja.list(toolset.getincludedirs(cfg, globalincludes, cfg.sysincludedirs))
	local forceincludes =	ninja.list(toolset.getforceincludes(cfg)) -- TODO pch
	local ldflags =			ninja.list(table.join(toolset.getLibraryDirectories(cfg), toolset.getldflags(cfg), cfg.linkoptions))
	local libs =			""

	if toolset_name == "msc" then
		warnings = ninja.list(toolset.getwarnings(cfg))

		-- for some reason Visual Studio add this libraries as "defaults" and premake doesn't tell us this
		default_msvc_libs = " kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib "
	end

	-- we don't pass getlinks(cfg) through dependencies
	-- because system libraries are often not in PATH so ninja can't find them
	libs = ninja.list(p.esc(config.getlinks(cfg, "siblings", "fullpath")))

	-- experimental feature, change install_name of shared libs
	--if (toolset_name == "clang") and (cfg.kind == p.SHAREDLIB) and ninja.endsWith(cfg.buildtarget.name, ".dylib") then
	--	ldflags = ldflags .. " -install_name " .. cfg.buildtarget.name
	--end

	local all_cflags = buildopt .. cflags .. warnings .. defines .. includes .. forceincludes
	local all_cxxflags = buildopt .. cflags .. cppflags .. cxxflags .. warnings .. defines .. includes .. forceincludes
	local all_ldflags = buildopt .. ldflags

	local obj_dir = project.getrelative(cfg.workspace, cfg.objdir)

	---------------------------------------------------- write rules
	p.w("# core rules for " .. cfg.name)
	if toolset_name == "msc" then -- TODO /NOLOGO is invalid, we need to use /nologo
		p.w("rule cc")
		p.w("  command = " .. cc .. all_cflags .. " /nologo /showIncludes -c $in /Fo$out")
		p.w("  description = cc $out")
		p.w("  deps = msvc")
		p.w("")
		p.w("rule cxx")
		p.w("  command = " .. cxx .. all_cxxflags .. " /nologo /showIncludes -c $in /Fo$out")
		p.w("  description = cxx $out")
		p.w("  deps = msvc")
		p.w("")
		p.w("rule ar")
		p.w("  command = " .. ar .. " $in /nologo -OUT:$out")
		p.w("  description = ar $out")
		p.w("")
		p.w("rule link")
		p.w("  command = " .. link .. " $in " .. ninja.list(ninja.shesc(toolset.getlinks(cfg))) .. default_msvc_libs .. " /link " .. all_ldflags .. " /nologo /out:$out")
		p.w("  description = link $out")
		p.w("")
	elseif toolset_name == "clang" then
		p.w("rule cc")
		p.w("  command = " .. cc .. all_cflags .. " -MMD -MF $out.d -c -o $out $in")
		p.w("  description = cc $out")
		p.w("  depfile = $out.d")
		p.w("  deps = gcc")
		p.w("")
		p.w("rule cxx")
		p.w("  command = " .. cxx .. all_cxxflags .. " -MMD -MF $out.d -c -o $out $in")
		p.w("  description = cxx $out")
		p.w("  depfile = $out.d")
		p.w("  deps = gcc")
		p.w("")
		p.w("rule ar")
		p.w("  command = " .. ar .. " rcs $out $in")
		p.w("  description = ar $out")
		p.w("")
		p.w("rule link")
		p.w("  command = " .. link .. " -o $out $in " .. ninja.list(ninja.shesc(toolset.getlinks(cfg, "system"))) .. " " .. all_ldflags)
		p.w("  description = link $out")
		p.w("")
	elseif toolset_name == "gcc" then
		p.w("rule cc")
		p.w("  command = " .. cc .. all_cflags .. " -MMD -MF $out.d -c -o $out $in")
		p.w("  description = cc $out")
		p.w("  depfile = $out.d")
		p.w("  deps = gcc")
		p.w("")
		p.w("rule cxx")
		p.w("  command = " .. cxx .. all_cxxflags .. " -MMD -MF $out.d -c -o $out $in")
		p.w("  description = cxx $out")
		p.w("  depfile = $out.d")
		p.w("  deps = gcc")
		p.w("")
		p.w("rule ar")
		p.w("  command = " .. ar .. " rcs $out $in")
		p.w("  description = ar $out")
		p.w("")
		p.w("rule link")
		p.w("  command = " .. link .. " -o $out $in " .. ninja.list(ninja.shesc(toolset.getlinks(cfg, "system"))) .. " " .. all_ldflags)
		p.w("  description = link $out")
		p.w("")
	end

	---------------------------------------------------- build all files
	p.w("# build files")
	local intermediateExt = function(cfg, var)
		if (var == "c") or (var == "cxx") then
			return iif(toolset_name == "msc", ".obj", ".o")
		elseif var == "res" then
			-- TODO
			return ".res"
		elseif var == "link" then
			return cfg.targetextension
		end
	end
	local objfiles = {}
	tree.traverse(project.getsourcetree(prj), {
	onleaf = function(node, depth)
		local filecfg = fileconfig.getconfig(node, cfg)
		if fileconfig.hasCustomBuildRule(filecfg) then
			-- TODO
		elseif path.iscppfile(node.abspath) then
			objfilename = obj_dir .. "/" .. node.objname .. intermediateExt(cfg, "cxx")
			objfiles[#objfiles + 1] = objfilename
			if ninja.endsWith(node.abspath, ".c") then
				p.w("build " .. p.esc(objfilename) .. ": cc " .. p.esc(node.abspath))
			else
				p.w("build " .. p.esc(objfilename) .. ": cxx " .. p.esc(node.abspath))
			end
		elseif path.isresourcefile(node.abspath) then
			-- TODO
		end
	end,
	}, false, 1)
	p.w("")

	---------------------------------------------------- build final target
	if cfg.kind == p.STATICLIB then
		p.w("# link static lib")
		p.w("build " .. p.esc(ninja.outputFilename(cfg)) .. ": ar " .. table.concat(p.esc(objfiles), " ") .. " " .. libs)

	elseif cfg.kind == p.SHAREDLIB then
		local output = ninja.outputFilename(cfg)
		p.w("# link shared lib")
		p.w("build " .. p.esc(output) .. ": link " .. table.concat(p.esc(objfiles), " ") .. " " .. libs)

		-- TODO I'm a bit confused here, previous build statement builds .dll/.so file
		-- but there are like no obvious way to tell ninja that .lib/.a is also build there
		-- and we use .lib/.a later on as dependency for linkage
		-- so let's create phony build statements for this, not sure if it's the best solution
		-- UPD this can be fixed by https://github.com/martine/ninja/pull/989
		if ninja.endsWith(output, ".dll") then
			p.w("build " .. p.esc(ninja.noext(output, ".dll")) .. ".lib: phony " .. p.esc(output))
		elseif ninja.endsWith(output, ".so") then
			p.w("build " .. p.esc(ninja.noext(output, ".so")) .. ".a: phony " .. p.esc(output))
		elseif ninja.endsWith(output, ".dylib") then
			-- but in case of .dylib there are no corresponding .a file
		else
			p.error("unknown type of shared lib '" .. output .. "', so no idea what to do, sorry")
		end

	elseif (cfg.kind == p.CONSOLEAPP) or (cfg.kind == p.WINDOWEDAPP) then
		p.w("# link executable")
		p.w("build " .. p.esc(ninja.outputFilename(cfg)) .. ": link " .. table.concat(p.esc(objfiles), " ") .. " " .. libs)

	else
		p.error("ninja action doesn't support this kind of target " .. cfg.kind)
	end

	p.w("")
end

-- return name of output binary relative to build folder
function ninja.outputFilename(cfg)
	return project.getrelative(cfg.workspace, cfg.buildtarget.directory) .. "/" .. cfg.buildtarget.name
end

-- return name of build file for configuration
function ninja.projectCfgFilename(cfg, relative)
	if relative ~= nil then
		relative = project.getrelative(cfg.workspace, cfg.location) .. "/"
	else
		relative = ""
	end
	
	local ninjapath = relative .. "build_" .. cfg.project.name  .. "_" .. cfg.buildcfg
	
	if cfg.platform ~= nil then ninjapath = ninjapath .. "_" .. cfg.platform end
	
	return ninjapath .. ".ninja"
end

-- check if string starts with string
function ninja.startsWith(str, starts)
	return str:sub(0, starts:len()) == starts
end

-- check if string ends with string
function ninja.endsWith(str, ends)
	return str:sub(-ends:len()) == ends
end

-- removes extension from string
function ninja.noext(str, ext)
	return str:sub(0, str:len() - ext:len())
end

-- generate all build files for every project configuration
function ninja.generateProject(prj)
	for cfg in project.eachconfig(prj) do
		p.generate(cfg, ninja.projectCfgFilename(cfg), ninja.generateProjectCfg)
	end
end

include("_preload.lua")

return ninja
