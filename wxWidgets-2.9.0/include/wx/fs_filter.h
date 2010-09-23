/////////////////////////////////////////////////////////////////////////////
// Name:        wx/fs_filter.h
// Purpose:     Filter file system handler
// Author:      Mike Wetherell
// Copyright:   (c) 2006 Mike Wetherell
// CVS-ID:      $Id: fs_filter.h 58757 2009-02-08 11:45:59Z VZ $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_FS_FILTER_H_
#define _WX_FS_FILTER_H_

#include "wx/defs.h"

#if wxUSE_FILESYSTEM

#include "wx/filesys.h"

//---------------------------------------------------------------------------
// wxFilterFSHandler
//---------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxFilterFSHandler : public wxFileSystemHandler
{
public:
    wxFilterFSHandler() : wxFileSystemHandler() { }
    virtual ~wxFilterFSHandler() { }

    virtual bool CanOpen(const wxString& location);
    virtual wxFSFile* OpenFile(wxFileSystem& fs, const wxString& location);

    virtual wxString FindFirst(const wxString& spec, int flags = 0);
    virtual wxString FindNext();

private:
    wxDECLARE_NO_COPY_CLASS(wxFilterFSHandler);
};

#endif // wxUSE_FILESYSTEM

#endif // _WX_FS_FILTER_H_
