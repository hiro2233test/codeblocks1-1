/*
* This file is part of Code::Blocks Studio, an open-source cross-platform IDE
* Copyright (C) 2003  Yiannis An. Mandravellos
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* Contact e-mail: Yiannis An. Mandravellos <mandrav@codeblocks.org>
* Program URL   : http://www.codeblocks.org
*
* $Id$
* $Date$
*/

#include "projectbuildtarget.h" // class's header file
#include "cbproject.h"
#include "compilerfactory.h"

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(FilesList);

// class constructor
ProjectBuildTarget::ProjectBuildTarget()
{
    m_BuildWithAll = true;
}

// class destructor
ProjectBuildTarget::~ProjectBuildTarget()
{
}

bool ProjectBuildTarget::GetIncludeInTargetAll()
{
	return m_BuildWithAll;
}

void ProjectBuildTarget::SetIncludeInTargetAll(bool buildIt)
{
	m_BuildWithAll = buildIt;
	SetModified(true);
}

//// PROJECTFILE //////////////////////

ProjectFile::ProjectFile()
{
}

ProjectFile::~ProjectFile()
{
    ClearBreakpoints();
}

void ProjectFile::AddBuildTarget(const wxString& targetName)
{
    if (buildTargets.Index(targetName) == wxNOT_FOUND)
        buildTargets.Add(targetName);
}

void ProjectFile::RenameBuildTarget(const wxString& oldTargetName, const wxString& newTargetName)
{
    int idx = buildTargets.Index(oldTargetName);
    if (idx != wxNOT_FOUND)
        buildTargets[idx] = newTargetName;
}

void ProjectFile::RemoveBuildTarget(const wxString& targetName)
{
    buildTargets.Remove(targetName);
}

bool ProjectFile::ShowOptions(wxWindow* parent)
{
    ProjectFileOptionsDlg dlg(parent, this);
    return dlg.ShowModal() == wxID_OK;
}

void ProjectFile::ClearBreakpoints()
{
    for (unsigned int i = 0; i < breakpoints.GetCount(); ++i)
    {
        DebuggerBreakpoint* bp = breakpoints[i];
        delete bp;
    }
    breakpoints.Clear();
}

DebuggerBreakpoint* ProjectFile::HasBreakpoint(int line)
{
    for (unsigned int i = 0; i < breakpoints.GetCount(); ++i)
    {
        DebuggerBreakpoint* bp = breakpoints[i];
        if (bp->line == line)
            return bp;
    }
    return 0L;
}

void ProjectFile::SetBreakpoint(int line)
{
    if (HasBreakpoint(line))
        return;
    DebuggerBreakpoint* bp = new DebuggerBreakpoint;
    bp->line = line;
    bp->ignoreCount = 0;
    bp->enabled = true;
    breakpoints.Add(bp);

}

void ProjectFile::RemoveBreakpoint(int line)
{
    DebuggerBreakpoint* bp = HasBreakpoint(line);
    if (bp)
    {
        breakpoints.Remove(bp);
        delete bp;
    }
}

void ProjectFile::ToggleBreakpoint(int line)
{
    DebuggerBreakpoint* bp = HasBreakpoint(line);
    if (bp)
        RemoveBreakpoint(line);
    else
        SetBreakpoint(line);
}

wxString ProjectFile::GetBaseName()
{
    wxFileName fname(relativeFilename);
    fname.SetExt("");
    return fname.GetFullPath();
}

const wxString& ProjectFile::GetObjName()
{
    if (m_ObjName.IsEmpty())
        SetObjName(relativeFilename);
    return m_ObjName;
}

void ProjectFile::SetObjName(const wxString& name)
{
    wxFileName fname(name);
    fname.SetExt(CompilerFactory::Compilers[project->GetCompilerIndex()]->GetSwitches().objectExtension);
    m_ObjName = fname.GetFullPath();
}
