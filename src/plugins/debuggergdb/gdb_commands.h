#ifndef GDB_DEBUGGER_COMMANDS_H
#define GDB_DEBUGGER_COMMANDS_H

#include <wx/string.h>
#include <wx/regex.h>
#include <wx/tipwin.h>
#include <globals.h>
#include <manager.h>
#include "debugger_defs.h"
#include "debuggergdb.h"
#include "debuggertree.h"
#include "backtracedlg.h"

//#0 wxEntry () at main.cpp:5
//#8  0x77d48734 in USER32!GetDC () from C:\WINDOWS\system32\user32.dll
//#9  0x001b04fe in ?? ()
//#29 0x100b07bc in wxEntry () from C:\WINDOWS\system32\wxmsw26_gcc_cb.dll
//#30 0x00403c0a in WinMain (hInstance=0x400000, hPrevInstance=0x0, lpCmdLine=0x241ef9 "", nCmdShow=10) at C:/Devel/wxSmithTest/app.cpp:297
//#31 0x004076ca in main () at C:/Devel/wxWidgets-2.6.1/include/wx/intl.h:555
static wxRegEx reBT1(_T("#([0-9]+)[ \t]+[0x]*([A-Fa-f0-9]*)[ \t]*[in]*[ \t]*([^( \t]+)[ \t]+(\\([^)]*\\))"));
static wxRegEx reBT2(_T("\\)[ \t]+[atfrom]+[ \t]+(.*):([0-9]+)"));
static wxRegEx reBT3(_T("\\)[ \t]+[atfrom]+[ \t]+(.*)"));
// Breakpoint 1 at 0x4013d6: file main.cpp, line 8.
static wxRegEx reBreakpoint(_T("Breakpoint ([0-9]+) at (0x[0-9A-Fa-f]+)"));
// eax            0x40e66666       1088841318
static wxRegEx reRegisters(_T("([A-z0-9]+)[ \t]+(0x[0-9A-z]+)"));
// 0x00401390 <main+0>:	push   ebp
static wxRegEx reDisassembly(_T("(0x[0-9A-Za-z]+)[ \t]+<.*>:[ \t]+(.*)"));
//Stack level 0, frame at 0x22ff80:
// eip = 0x401497 in main (main.cpp:16); saved eip 0x4011e7
// source language c++.
// Arglist at 0x22ff78, args: argc=1, argv=0x3e3cb0
// Locals at 0x22ff78, Previous frame's sp is 0x22ff80
// Saved registers:
//  ebx at 0x22ff6c, ebp at 0x22ff78, esi at 0x22ff70, edi at 0x22ff74, eip at 0x22ff7c
static wxRegEx reDisassemblyInit(_T("^Stack level [0-9]+, frame at (0x[A-Fa-f0-9]+):"));
static wxRegEx reDisassemblyInitFunc(_T("eip = (0x[A-Fa-f0-9]+) in ([^;]*)"));
// wxString and wxChar types regexes
static wxRegEx reWXString(_T("[^[:alnum:]_]*wxString[^[:alnum:]_]*"));
static wxRegEx reWXChar(_T("[^[:alnum:]_]*wxChar[^[:alnum:]_]*"));

// convenience function
wxString ParseWXStringOutput(const wxString& output)
{
    // unicode wxString is a special case. The debugger will return something like this:
    // {38 '&', 69 'E', 110 'n', 97 'a', 98 'b', 108 'l', 101 'e'}
    // So, we quickly parse it and display the chars as a nice string :)
    wxString w;
    w << _T('"');
    size_t len = output.Len();
    size_t c = 0;
    while (c < len)
    {
        switch (output[c])
        {
            case _T('\''):
                w << output[c + 1];
                c += 2;
                break;

            default:
                break;
        }
        ++c;
    }
    w << _T('\"');
    return w;
}

/**
  * Command to add a search directory for source files in debugger's paths.
  */
class GdbCmd_AddSourceDir : public DebuggerCmd
{
    public:
        /** If @c dir is empty, resets all search dirs to $cdir:$cwd, the default. */
        GdbCmd_AddSourceDir(DebuggerDriver* driver, const wxString& dir)
            : DebuggerCmd(driver)
        {
            m_Cmd << _T("directory ") << dir;
        }
        void ParseOutput(const wxString& output)
        {
            // Output:
            // Warning: C:\Devel\tmp\console\111: No such file or directory.
            // Source directories searched: <dir>;$cdir;$cwd
            if (output.StartsWith(_T("Warning: ")))
                m_pDriver->Log(output.BeforeFirst(_T('\n')));
        }
};

/**
  * Command to the set the file to be debugged.
  */
class GdbCmd_SetDebuggee : public DebuggerCmd
{
    public:
        /** @param file The file to debug. */
        GdbCmd_SetDebuggee(DebuggerDriver* driver, const wxString& file)
            : DebuggerCmd(driver)
        {
            m_Cmd << _T("file ") << file;
        }
        void ParseOutput(const wxString& output)
        {
            // Output:
            // Reading symbols from C:\Devel\tmp\console/console.exe...done.
            // or if it doesn't exist:
            // console.exe: No such file or directory.

            // just log everything before the prompt
            m_pDriver->Log(output.BeforeFirst(_T('\n')));
        }
};

/**
  * Command to the add symbol files.
  */
class GdbCmd_AddSymbolFile : public DebuggerCmd
{
    public:
        /** @param file The file which contains the symbols. */
        GdbCmd_AddSymbolFile(DebuggerDriver* driver, const wxString& file)
            : DebuggerCmd(driver)
        {
            m_Cmd << _T("add-symbol-file ") << file;
        }
        void ParseOutput(const wxString& output)
        {
            // Output:
            //
            // add symbol table from file "console.exe" at
            // Reading symbols from C:\Devel\tmp\console/console.exe...done.
            //
            // or if it doesn't exist:
            // add symbol table from file "console.exe" at
            // console.exe: No such file or directory.

            // just ignore the "add symbol" line and log the rest before the prompt
            m_pDriver->Log(output.AfterFirst(_T('\n')).BeforeLast(_T('\n')));
        }
};

/**
  * Command to set the arguments to the debuggee.
  */
class GdbCmd_SetArguments : public DebuggerCmd
{
    public:
        /** @param file The file which contains the symbols. */
        GdbCmd_SetArguments(DebuggerDriver* driver, const wxString& args)
            : DebuggerCmd(driver)
        {
            m_Cmd << _T("set args ") << args;
        }
        void ParseOutput(const wxString& output)
        {
            // No output
        }
};

/**
  * Command to the attach to a process.
  */
class GdbCmd_AttachToProcess : public DebuggerCmd
{
    public:
        /** @param file The file to debug. */
        GdbCmd_AttachToProcess(DebuggerDriver* driver, int pid)
            : DebuggerCmd(driver)
        {
            m_Cmd << _T("attach ") << wxString::Format(_T("%d"), pid);
        }
        void ParseOutput(const wxString& output)
        {
            // Output:
            // Attaching to process <pid>
            // or,
            // Can't attach to process.
            wxArrayString lines = GetArrayFromString(output, _T('\n'));
    		for (unsigned int i = 0; i < lines.GetCount(); ++i)
    		{
                if (lines[i].StartsWith(_T("Attaching")))
                    m_pDriver->Log(lines[i]);
                else if (lines[i].StartsWith(_T("Can't ")))
                {
                    // log this and quit debugging
                    m_pDriver->Log(lines[i]);
                    m_pDriver->QueueCommand(new DebuggerCmd(m_pDriver, _T("quit")));
                }
//                m_pDriver->DebugLog(lines[i]);
    		}
        }
};

/**
  * Command to the detach from the process.
  */
class GdbCmd_Detach : public DebuggerCmd
{
    public:
        /** @param file The file to debug. */
        GdbCmd_Detach(DebuggerDriver* driver)
            : DebuggerCmd(driver)
        {
            m_Cmd << _T("detach");
        }
        void ParseOutput(const wxString& output)
        {
            // Output:
            // Attaching to process <pid>
            wxArrayString lines = GetArrayFromString(output, _T('\n'));
    		for (unsigned int i = 0; i < lines.GetCount(); ++i)
    		{
                if (lines[i].StartsWith(_T("Detaching")))
                    m_pDriver->Log(lines[i]);
//                m_pDriver->DebugLog(lines[i]);
    		}
        }
};

/**
  * Command to add a breakpoint.
  */
class GdbCmd_AddBreakpoint : public DebuggerCmd
{
    public:
        /** @param bp The breakpoint to set. */
        GdbCmd_AddBreakpoint(DebuggerDriver* driver, DebuggerBreakpoint* bp)
            : DebuggerCmd(driver),
            m_BP(bp)
        {
            if (m_BP->enabled)
            {
                if (m_BP->func.IsEmpty())
                {
                    wxString out = m_BP->filename;
                    DebuggerGDB::ConvertToGDBFile(out);
                    QuoteStringIfNeeded(out);
                    // we add one to line,  because scintilla uses 0-based line numbers, while gdb uses 1-based
                    if (!m_BP->temporary)
                        m_Cmd << _T("break ");
                    else
                        m_Cmd << _T("tbreak ");
                    m_Cmd << out << _T(":") << wxString::Format(_T("%d"), m_BP->line + 1);
                }
                //GDB workaround
                //Use function name if this is C++ constructor/destructor
                else
                {
                    if (!m_BP->temporary)
                        m_Cmd << _T("break ");
                    else
                        m_Cmd << _T("tbreak ");
                    m_Cmd << m_BP->func;
                }
                //end GDB workaround

                m_BP->alreadySet = true;
                // condition and ignore count will be set in ParseOutput, where we 'll have the bp number
            }
        }
        void ParseOutput(const wxString& output)
        {
            // possible outputs (we 're only interested in 1st sample):
            //
            // Breakpoint 1 at 0x4013d6: file main.cpp, line 8.
            // No line 100 in file "main.cpp".
            // No source file named main2.cpp.
            if (reBreakpoint.Matches(output))
            {
//                m_pDriver->DebugLog(wxString::Format(_("Breakpoint added: file %s, line %d"), m_BP->filename.c_str(), m_BP->line + 1));
                if (!m_BP->func.IsEmpty())
                    m_pDriver->DebugLog(_("(work-around for constructors activated)"));

                reBreakpoint.GetMatch(output, 1).ToLong(&m_BP->bpNum);
                reBreakpoint.GetMatch(output, 2).ToULong(&m_BP->address, 16);

                // conditional breakpoint
                if (m_BP->useCondition && !m_BP->condition.IsEmpty())
                {
                    wxString cmd;
                    cmd << _T("condition ") << wxString::Format(_T("%d"), (int) m_BP->bpNum) << _T(" ") << m_BP->condition;
                    m_pDriver->QueueCommand(new DebuggerCmd(m_pDriver, cmd), DebuggerDriver::High);
                }

                // ignore count
                if (m_BP->useIgnoreCount && m_BP->ignoreCount > 0)
                {
                    wxString cmd;
                    cmd << _T("ignore ") << wxString::Format(_T("%d"), (int) m_BP->bpNum) << _T(" ") << m_BP->ignoreCount;
                    m_pDriver->QueueCommand(new DebuggerCmd(m_pDriver, cmd), DebuggerDriver::High);
                }
            }
            else
                m_pDriver->Log(output); // one of the error responses
        }

        DebuggerBreakpoint* m_BP;
};

/**
  * Command to remove a breakpoint.
  */
class GdbCmd_RemoveBreakpoint : public DebuggerCmd
{
    public:
        /** @param bp The breakpoint to remove. If NULL, all breakpoints are removed. */
        GdbCmd_RemoveBreakpoint(DebuggerDriver* driver, DebuggerBreakpoint* bp)
            : DebuggerCmd(driver),
            m_BP(bp)
        {
            if (!bp)
            {
                m_Cmd << _T("delete");
                return;
            }

            if (bp->enabled && bp->bpNum > 0)
            {
                m_Cmd << _T("delete ") << wxString::Format(_T("%d"), (int) bp->bpNum);
            }
        }
        void ParseOutput(const wxString& output)
        {
            if (!m_BP)
                return;

            // invalidate bp number
            m_BP->bpNum = -1;

            if (!output.IsEmpty())
                m_pDriver->Log(output);
//            m_pDriver->DebugLog(wxString::Format(_("Breakpoint removed: file %s, line %d"), m_BP->filename.c_str(), m_BP->line + 1));
        }

        DebuggerBreakpoint* m_BP;
};

/**
  * Command to get info about local frame variables.
  */
class GdbCmd_InfoLocals : public DebuggerCmd
{
        DebuggerTree* m_pDTree;
    public:
        /** @param tree The tree to display the locals. */
        GdbCmd_InfoLocals(DebuggerDriver* driver, DebuggerTree* dtree)
            : DebuggerCmd(driver),
            m_pDTree(dtree)
        {
            m_Cmd << _T("info locals");
        }
        void ParseOutput(const wxString& output)
        {
            wxArrayString lines = GetArrayFromString(output, _T('\n'));
            wxString locals;
    		locals << _T("Local variables = {");
    		for (unsigned int i = 0; i < lines.GetCount(); ++i)
                locals << lines[i] << _T(',');
            locals << _T("}") << _T('\n');
            m_pDTree->BuildTree(0, locals, wsfGDB);
        }
};

/**
  * Command to get info about current function arguments.
  */
class GdbCmd_InfoArguments : public DebuggerCmd
{
        DebuggerTree* m_pDTree;
    public:
        /** @param tree The tree to display the args. */
        GdbCmd_InfoArguments(DebuggerDriver* driver, DebuggerTree* dtree)
            : DebuggerCmd(driver),
            m_pDTree(dtree)
        {
            m_Cmd << _T("info args");
        }
        void ParseOutput(const wxString& output)
        {
            wxArrayString lines = GetArrayFromString(output, _T('\n'));
            wxString args;
    		args << _T("Function Arguments = {");
    		for (unsigned int i = 0; i < lines.GetCount(); ++i)
                args << lines[i] << _T(',');
            args << _T("}") << _T('\n');
            m_pDTree->BuildTree(0, args, wsfGDB);
        }
};

/**
  * Command to get info about a watched variable.
  */
class GdbCmd_Watch : public DebuggerCmd
{
        DebuggerTree* m_pDTree;
        Watch* m_pWatch;
    public:
        /** @param tree The tree to display the watch. */
        GdbCmd_Watch(DebuggerDriver* driver, DebuggerTree* dtree, Watch* watch)
            : DebuggerCmd(driver),
            m_pDTree(dtree),
            m_pWatch(watch)
        {
            m_Cmd << _T("output ");
            switch (m_pWatch->format)
            {
                case Decimal:       m_Cmd << _T("/d "); break;
                case Unsigned:      m_Cmd << _T("/u "); break;
                case Hex:           m_Cmd << _T("/x "); break;
                case Binary:        m_Cmd << _T("/t "); break;
                case Char:          m_Cmd << _T("/c "); break;
                case cbWXString:    m_Cmd = _T("print_wxstring "); break;
                default:            break;
            }
            m_Cmd << m_pWatch->keyword;
        }
        void ParseOutput(const wxString& output)
        {
            wxArrayString lines = GetArrayFromString(output, _T('\n'));
            wxString w;
    		w << m_pWatch->keyword << _T(" = ");
    		for (unsigned int i = 0; i < lines.GetCount(); ++i)
    		{
    		    if (m_pWatch->format == cbWXString)
                    w << ParseWXStringOutput(lines[i]);
    		    else
                    w << lines[i];
                w << _T(',');
    		}
            w << _T('\n');
            m_pDTree->BuildTree(m_pWatch, w, wsfGDB);
        }
};

/**
  * Command to get a watched variable's type.
  */
class GdbCmd_FindWatchType : public DebuggerCmd
{
        DebuggerTree* m_pDTree;
        Watch* m_pWatch;
    public:
        /** @param tree The tree to display the watch. */
        GdbCmd_FindWatchType(DebuggerDriver* driver, DebuggerTree* dtree, Watch* watch)
            : DebuggerCmd(driver),
            m_pDTree(dtree),
            m_pWatch(watch)
        {
            m_Cmd << _T("whatis ");
            m_Cmd << m_pWatch->keyword;
        }
        void ParseOutput(const wxString& output)
        {
            // examples:
            // type = wxString
            // type = const wxChar
            // type = Action *
            // type = bool

            wxString tmp = output.AfterFirst(_T('='));
            if (reWXString.Matches(tmp))
                m_pWatch->format = cbWXString;
            else if (reWXChar.Matches(tmp))
                m_pWatch->format = Char;
            // in any case, actually add this watch with high priority
            m_pDriver->QueueCommand(new GdbCmd_Watch(m_pDriver, m_pDTree, m_pWatch), DebuggerDriver::High);
        }
};

/**
  * Command to display a tooltip about a variables value.
  */
class GdbCmd_TooltipEvaluation : public DebuggerCmd
{
        wxTipWindow** m_pWin;
        wxRect m_WinRect;
        wxString m_What;
        bool m_IsWXString;
    public:
        /** @param what The variable to evaluate.
            @param win A pointer to the tip window pointer.
            @param tiprect The tip window's rect.
        */
        GdbCmd_TooltipEvaluation(DebuggerDriver* driver, const wxString& what, wxTipWindow** win, const wxRect& tiprect, bool isWXString = false)
            : DebuggerCmd(driver),
            m_pWin(win),
            m_WinRect(tiprect),
            m_What(what),
            m_IsWXString(isWXString)
        {
            m_Cmd << (isWXString ? _T("print_wxstring ") : _T("output ")) << what;
        }
        void ParseOutput(const wxString& output)
        {
            wxString tip;
            if (output.StartsWith(_T("No symbol ")) || output.StartsWith(_T("Attempt to ")))
                tip = output;
            else
            {
                tip = m_What + _T("=");
                if (m_IsWXString)
                    tip << ParseWXStringOutput(output);
                else
                    tip << output;
            }

            if (*m_pWin)
                (*m_pWin)->Destroy();
            *m_pWin = new wxTipWindow(Manager::Get()->GetAppWindow(), tip, 640, m_pWin, &m_WinRect);
//            m_pDriver->DebugLog(output);
        }
};

/**
  * Command to get a symbol's type and use it for tooltip evaluation.
  */
class GdbCmd_FindTooltipType : public DebuggerCmd
{
        wxTipWindow** m_pWin;
        wxRect m_WinRect;
        wxString m_What;
    public:
        /** @param tree The tree to display the watch. */
        GdbCmd_FindTooltipType(DebuggerDriver* driver, const wxString& what, wxTipWindow** win, const wxRect& tiprect)
            : DebuggerCmd(driver),
            m_pWin(win),
            m_WinRect(tiprect),
            m_What(what)
        {
            m_Cmd << _T("whatis ");
            m_Cmd << m_What;
        }
        void ParseOutput(const wxString& output)
        {
            // examples:
            // type = wxString
            // type = const wxChar
            // type = Action *
            // type = bool

            wxString tmp = output.AfterFirst(_T('='));
            tmp.Trim(false);
            tmp.Trim(true);
            bool isWXString = reWXString.Matches(tmp);
            // in any case, actually add this watch with high priority
            m_pDriver->QueueCommand(new GdbCmd_TooltipEvaluation(m_pDriver, m_What, m_pWin, m_WinRect, isWXString), DebuggerDriver::High);
        }
};

/**
  * Command to run a backtrace.
  */
class GdbCmd_Backtrace : public DebuggerCmd
{
        BacktraceDlg* m_pDlg;
    public:
        /** @param dlg The backtrace dialog. */
        GdbCmd_Backtrace(DebuggerDriver* driver, BacktraceDlg* dlg)
            : DebuggerCmd(driver),
            m_pDlg(dlg)
        {
            m_Cmd << _T("bt 30");
        }
        void ParseOutput(const wxString& output)
        {
            m_pDlg->Clear();
            wxArrayString lines = GetArrayFromString(output, _T('\n'));
    		for (unsigned int i = 0; i < lines.GetCount(); ++i)
    		{
    		    // reBT1 matches frame number, address, function and args (common to all formats)
    		    // reBT2 matches filename and line (optional)
    		    // reBT3 matches filename only (for DLLs) (optional)

    		    // #0  main (argc=1, argv=0x3e2440) at my main.cpp:15
    		    if (reBT1.Matches(lines[i]))
    		    {
//                    m_pDriver->DebugLog(_T("MATCH!"));
                    StackFrame sf;
                    sf.valid = true;
    		        reBT1.GetMatch(lines[i], 1).ToLong(&sf.number);
    		        reBT1.GetMatch(lines[i], 2).ToULong(&sf.address, 16);
    		        sf.function = reBT1.GetMatch(lines[i], 3) + reBT1.GetMatch(lines[i], 4);
                    if (reBT2.Matches(lines[i]))
                    {
                        sf.file = reBT2.GetMatch(lines[i], 1);
                        sf.line = reBT2.GetMatch(lines[i], 2);
                    }
                    else if (reBT3.Matches(lines[i]))
                        sf.file = reBT3.GetMatch(lines[i], 1);
                    m_pDlg->AddFrame(sf);
    		    }
    		}
//            m_pDriver->DebugLog(output);
        }
};

/**
  * Command to run a disassembly. Use this instead of GdbCmd_DisassemblyInit, which is chained-called.
  */
class GdbCmd_InfoRegisters : public DebuggerCmd
{
        CPURegistersDlg* m_pDlg;
    public:
        /** @param dlg The disassembly dialog. */
        GdbCmd_InfoRegisters(DebuggerDriver* driver, CPURegistersDlg* dlg)
            : DebuggerCmd(driver),
            m_pDlg(dlg)
        {
            m_Cmd << _T("info registers");
        }
        void ParseOutput(const wxString& output)
        {
            // output is a series of:
            //
            // eax            0x40e66666       1088841318
            // ecx            0x40cbf0 4246512
            // edx            0x77c61ae8       2009471720
            // ebx            0x4000   16384
            // esp            0x22ff50 0x22ff50
            // ebp            0x22ff78 0x22ff78
            // esi            0x22ef80 2289536
            // edi            0x5dd3f4 6149108
            // eip            0x4013c9 0x4013c9
            // eflags         0x247    583
            // cs             0x1b     27
            // ss             0x23     35
            // ds             0x23     35
            // es             0x23     35
            // fs             0x3b     59
            // gs             0x0      0

            if (!m_pDlg)
                return;

            wxArrayString lines = GetArrayFromString(output, _T('\n'));
    		for (unsigned int i = 0; i < lines.GetCount(); ++i)
    		{
                if (reRegisters.Matches(lines[i]))
                {
                    long int addr;
                    reRegisters.GetMatch(lines[i], 2).ToLong(&addr, 16);
                    m_pDlg->SetRegisterValue(CPURegistersDlg::RegisterIndexFromName(reRegisters.GetMatch(lines[i], 1)), addr);
                }
    		}
//            m_pDlg->Show(true);
//            m_pDriver->DebugLog(output);
        }
};

/**
  * Command to run a disassembly.
  */
class GdbCmd_Disassembly : public DebuggerCmd
{
        DisassemblyDlg* m_pDlg;
    public:
        /** @param dlg The disassembly dialog. */
        GdbCmd_Disassembly(DebuggerDriver* driver, DisassemblyDlg* dlg)
            : DebuggerCmd(driver),
            m_pDlg(dlg)
        {
            m_Cmd << _T("disassemble");
        }
        void ParseOutput(const wxString& output)
        {
            // output is a series of:
            //
            // Dump of assembler code for function main:
            // 0x00401390 <main+0>:	push   ebp
            // ...
            // End of assembler dump.

            if (!m_pDlg)
                return;

            wxArrayString lines = GetArrayFromString(output, _T('\n'));
    		for (unsigned int i = 0; i < lines.GetCount(); ++i)
    		{
                if (reDisassembly.Matches(lines[i]))
                {
                    long int addr;
                    reDisassembly.GetMatch(lines[i], 1).ToLong(&addr, 16);
                    m_pDlg->AddAssemblerLine(addr, reDisassembly.GetMatch(lines[i], 2));
                }
    		}
//            m_pDlg->Show(true);
//            m_pDriver->DebugLog(output);
        }
};

/**
  * Command to initialize a disassembly. Use this instead of GdbCmd_Disassembly, which is chain-called by this.
  */
class GdbCmd_DisassemblyInit : public DebuggerCmd
{
        DisassemblyDlg* m_pDlg;
        static wxString LastAddr;
    public:
        /** @param dlg The disassembly dialog. */
        GdbCmd_DisassemblyInit(DebuggerDriver* driver, DisassemblyDlg* dlg)
            : DebuggerCmd(driver),
            m_pDlg(dlg)
        {
            m_Cmd << _T("info frame");
        }
        void ParseOutput(const wxString& output)
        {
            if (!m_pDlg)
                return;

            if (reDisassemblyInit.Matches(output))
            {
                StackFrame sf;
                wxString addr = reDisassemblyInit.GetMatch(output, 1);
                if (addr == LastAddr)
                    return;
                LastAddr = addr;
                addr.ToLong((long int*)&sf.address, 16);

                if (reDisassemblyInitFunc.Matches(output))
                {
                    sf.function = reDisassemblyInitFunc.GetMatch(output, 2);
                    long int active;
                    reDisassemblyInitFunc.GetMatch(output, 1).ToLong(&active, 16);
                    m_pDlg->SetActiveAddress(active);
                }

                sf.valid = true;
                m_pDlg->Clear(sf);
                m_pDriver->QueueCommand(new GdbCmd_Disassembly(m_pDriver, m_pDlg)); // chain call
            }
//            m_pDriver->DebugLog(output);
        }
};
wxString GdbCmd_DisassemblyInit::LastAddr;

#endif // DEBUGGER_COMMANDS_H
