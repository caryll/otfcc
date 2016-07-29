---
-- xcode/xcode4_scheme.lua
-- Generate a shared scheme for an Xcode C/C++ project.
-- Copyright (c) 2009-2016 Jason Perkins and the Premake project
---

local p = premake
local m = p.modules.xcode_alt

local project = p.project
local config = p.config
local fileconfig = p.fileconfig
local tree = p.tree

m.scheme = {}
local scheme = m.scheme

function scheme.Header(prj)
	p.w('<?xml version="1.0" encoding="UTF-8"?>')
	p.push('<Scheme')
	p.w('LastUpgradeVersion = "0500"')
	p.w('version = "1.3">')
end

function scheme.Footer(prj)
	p.pop('</Scheme>')
end

function scheme.buildablereference(prj)
	local tr = project.getsourcetree(prj)
	for _, node in ipairs(tr.products.children) do
		p.push('<BuildableReference')
		p.w('BuildableIdentifier = "primary"')
		p.w('BlueprintIdentifier = "%s"', node.targetid)
		p.w('BuildableName = "%s"', node.name)
		p.w('BlueprintName = "%s"', tr.name)
		p.w('ReferencedContainer = "container:%s.xcodeproj">', tr.name)
		p.pop('</BuildableReference>')
	end
end

function scheme.build(prj)
	p.push('<BuildAction')
	p.w('parallelizeBuildables = "YES"')
	p.w('buildImplicitDependencies = "YES">')
	p.push('<BuildActionEntries>')
	p.push('<BuildActionEntry')
	p.w('buildForTesting = "YES"')
	p.w('buildForRunning = "YES"')
	p.w('buildForProfiling = "YES"')
	p.w('buildForArchiving = "YES"')
	p.w('buildForAnalyzing = "YES">')
	scheme.buildablereference(prj)
	p.pop('</BuildActionEntry>')
	p.pop('</BuildActionEntries>')
	p.pop('</BuildAction>')
end

function scheme.test(prj)
	p.push('<TestAction')
	p.w('selectedDebuggerIdentifier = "Xcode.DebuggerFoundation.Debugger.LLDB"')
	p.w('selectedLauncherIdentifier = "Xcode.DebuggerFoundation.Launcher.LLDB"')
	p.w('shouldUseLaunchSchemeArgsEnv = "YES"')
	p.w('buildConfiguration = "Debug">')
	p.push('<Testables>')
	p.pop('</Testables>')
	p.push('<MacroExpansion>')
	scheme.buildablereference(prj)
	p.pop('</MacroExpansion>')
	p.pop('</TestAction>')
end

function scheme.launch(prj)
	p.push('<LaunchAction')
	p.w('selectedDebuggerIdentifier = "Xcode.DebuggerFoundation.Debugger.LLDB"')
	p.w('selectedLauncherIdentifier = "Xcode.DebuggerFoundation.Launcher.LLDB"')
	p.w('launchStyle = "0"')
	p.w('useCustomWorkingDirectory = "NO"')
	p.w('buildConfiguration = "Debug"')
	p.w('ignoresPersistentStateOnLaunch = "NO"')
	p.w('debugDocumentVersioning = "NO"')
	p.w('allowLocationSimulation = "YES">')
	p.push('<BuildableProductRunnable>')
	scheme.buildablereference(prj)
	p.pop('</BuildableProductRunnable>')
	p.push('<AdditionalOptions>')
	p.pop('</AdditionalOptions>')
	p.pop('</LaunchAction>')
end

function scheme.profile(prj)
	p.push('<ProfileAction')
	p.w('shouldUseLaunchSchemeArgsEnv = "YES"')
	p.w('savedToolIdentifier = ""')
	p.w('useCustomWorkingDirectory = "NO"')
	p.w('buildConfiguration = "Release"')
	p.w('debugDocumentVersioning = "NO">')
	p.push('<BuildableProductRunnable>')
	scheme.buildablereference(prj)
	p.pop('</BuildableProductRunnable>')
	p.pop('</ProfileAction>')
end

function scheme.analyze(prj)
	p.push('<AnalyzeAction')
	p.w('buildConfiguration = "Debug">')
	p.pop('</AnalyzeAction>')
end

function scheme.archive(prj)
	p.push('<ArchiveAction')
	p.w('buildConfiguration = "Release"')
	p.w('revealArchiveInOrganizer = "YES">')
	p.pop('</ArchiveAction>')
end

--
-- Generate the shared data scheme for an xcode project
--
-- @param prj
--    The Premake project to generate.
--

m.scheme.project = function(prj)
	return {
		scheme.Header,
		scheme.build,
		scheme.test,
		scheme.launch,
		scheme.profile,
		scheme.analyze,
		scheme.archive,
		scheme.Footer,
	}
end

function m.scheme.generate(prj)
	p.callArray(m.scheme.project, prj)
end
