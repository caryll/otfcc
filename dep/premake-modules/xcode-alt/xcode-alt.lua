---
-- xcode/xcode-alt.lua
-- Common support code for the Apple Xcode exporters.
-- Copyright (c) 2009-2015 Jason Perkins and the Premake project
---

	local p = premake

	p.modules.xcode_alt = {}

	local m = p.modules.xcode_alt
	m._VERSION = p._VERSION
	m.elements = {}

	include("xcode_common.lua")
	include("xcode4_workspace.lua")
	include("xcode_project.lua")
	include("xcode_scheme.lua")

	newaction {
		trigger     = "xcode4",
		shortname   = "Apple Xcode 4 - Alt",
		description = "Generate Apple Xcode 4 project files",

		-- Xcode always uses Mac OS X path and naming conventions

		os = "macosx",

		-- The capabilities of this action

		valid_kinds     = { "ConsoleApp", "WindowedApp", "SharedLib", "StaticLib", "Makefile", "None" },
		valid_languages = { "C", "C++" },
		valid_tools     = {
			cc = { "gcc", "clang" },
		},

		-- Workspace and project generation logic

		onWorkspace = function(wks)
			p.generate(wks, ".xcworkspace/contents.xcworkspacedata", p.modules.xcode_alt.generateWorkspace)
		end,

		onProject = function(prj)
			p.generate(prj, ".xcodeproj/project.pbxproj", p.modules.xcode_alt.generateProject)
			p.generate(prj, ".xcodeproj/xcshareddata/xcschemes/" .. prj.name .. ".xcscheme", p.modules.xcode_alt.scheme.generate)
		end,
	}

	return m
