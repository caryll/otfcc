--
-- Name:        premake-ninja/_preload.lua
-- Purpose:     Define the ninja action.
-- Author:      Dmitry Ivanov
-- Created:     2015/07/04
-- Copyright:   (c) 2015 Dmitry Ivanov
--

	local p = premake
	local solution = premake.solution
	local project = premake.project

	newaction
	{
		-- Metadata for the command line and help system
		trigger			= "ninja",
		shortname		= "ninja",
		description		= "Ninja is a small build system with a focus on speed",
		module			= "ninja",

		-- The capabilities of this action
		valid_kinds		= {"ConsoleApp", "WindowedApp", "Makefile", "SharedLib", "StaticLib"}, -- TODO do we need MakeFile ?
		valid_languages	= {"C", "C++"},
		valid_tools		= {cc = { "gcc", "clang", "msc" }},

		-- Solution and project generation logic
		onSolution = function(sln)
			p.eol("\r\n")
			p.indent("  ")
			p.escaper(p.modules.ninja.esc)
			p.generate(sln, "build.ninja", p.modules.ninja.generateSolution)
		end,
		onProject = function(prj)
			p.eol("\r\n")
			p.indent("  ")
			p.escaper(p.modules.ninja.esc)
			p.modules.ninja.generateProject(prj)
		end,
		onBranch = function(prj)
			p.eol("\r\n")
			p.indent("  ")
			p.escaper(p.modules.ninja.esc)
			p.modules.ninja.generateProject(prj)
		end,
		onCleanSolution = function(sln)
			-- TODO
		end,
		onCleanProject = function(prj)
			-- TODO
		end,
		onCleanTarget = function(prj)
			-- TODO
		end,
	}


--
-- Decide when the full module should be loaded.
--

	return function(cfg)
		return (_ACTION == "ninja")
	end
