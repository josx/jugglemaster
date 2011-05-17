/*
 * JMDeluxe - Portable JuggleMaster based on wxWindows
 * (C) Gary Briggs 2003
 *
 * JuggleMaster is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */ 

#include "patt.h"
#include <wx/progdlg.h>
#ifndef __WXMSW__
    #include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>


/*PatternLoader::PatternLoader(wxWindow *p, int redownload) : 
 {
	can_use = 0;
	parent = p;
	styles.first = NULL;
	groups.first = NULL;
	if(OpenFile((const char *)DEFAULT_PATTERNFILE,redownload)) {
		can_use = ParseFile();
		CloseFile();
	}
}*/

/*PatternLoader::PatternLoader(const char *filename, wxWindow *p, int redownload) {
	can_use = 0;
	parent = p;
	styles.first = NULL;
	groups.first = NULL;
	if(OpenFile(filename,redownload)) {
		can_use = ParseFile();
		CloseFile();
	}
}*/

//Refactored constructor
PatternLoader::PatternLoader(wxWindow *p, const char *filename,
    const char *js_filename, int redownload) :
				parent(p),
				can_use(0),
				current_group(NULL),
				current_style(NULL),
				current_pattern(NULL) {
    styles.first = NULL;
    groups.first = NULL;
    
    patternfile = OpenFile(filename, redownload);
    if (js_filename) patternfile_js = OpenFile(js_filename, redownload);    

    can_use = ParseFiles();
    CloseFiles();
}


int PatternLoader::Usable() {
	return(can_use);
}

FILE* PatternLoader::OpenFile(const char *filename, int redownload) {
	wxString targetfilename;
	struct stat buf;
	targetfilename = wxGetHomeDir();
	if(targetfilename.Len() > 0) {
		targetfilename += wxT("/.jugglemaster/");
		if(!wxDirExists(targetfilename)) {
			if(!wxMkdir(targetfilename,0755)) {
				targetfilename = wxT("");
			}
		}
		targetfilename += wxString(filename,wxConvUTF8);
	} else {
		targetfilename = wxString(filename,wxConvUTF8);
	}

	if(stat((const char *)targetfilename.mb_str(wxConvUTF8),&buf) != -1 && !redownload) {
		return fopen((const char *)(targetfilename.mb_str(wxConvUTF8)),"r");
	} else if(stat(filename,&buf) != -1 && !redownload) {
		wxCopyFile(wxString(filename,wxConvUTF8),targetfilename);
		return fopen((const char *)targetfilename.mb_str(wxConvUTF8),"r");
	} else {
		wxString fullurl(WEB_PREFIX, wxConvUTF8);
		wxString proxy;
		wxString message;
		fullurl.Append(wxString(filename,wxConvUTF8));
		message.Printf(_T("Downloading File: %s\n"),(const char *)fullurl.c_str());
		unsigned int current_progress = 0;
		char buffer[1024];

		wxURL url(fullurl);

		if(wxGetEnv(wxT("http_proxy"),&proxy)) {
			if(proxy.Find(wxT("//")) > -1) {
				proxy = proxy.Mid(proxy.Find(wxT("//"))+2);
			}
			url.SetProxy(proxy);
		}

		wxInputStream *data = url.GetInputStream();
		// wxHTTP url;
		// wxInputStream *data = url.GetInputStream(fullurl);

		if ( data ) {
			wxProgressDialog progress(_T("Progress"),message,(int)data->GetSize());
			wxFileOutputStream outputfile(targetfilename);
			while(!data->Eof() && current_progress!=data->GetSize()) {
				data->Read((void *)buffer,1024);
				outputfile.Write((const void *)buffer,data->LastRead());
				current_progress+=data->LastRead();
				progress.Update(current_progress);
			}
			// data->Read(outputfile);
			// printf("Downloading Done\n");
			delete data;
		} else {
			wxMessageDialog errordlg(parent,_T("An error occured while downloading"),_T("Error"),wxOK|wxICON_ERROR);
			errordlg.ShowModal();
		}
        return fopen((const char *)(targetfilename.mb_str(wxConvUTF8)),"r");
	}
}

#include "sqlite_patterns.h"

int PatternLoader::ParseFiles() {
  static bool testsql = true;

  if (testsql) {
    JMPatterns* p = new JMPatterns();
    p->initializeDatabase(NULL, patternfile, patternfile_js);
    //rewind(patternfile);
    //rewind(patternfile_js);
    testsql = false;
  }

	return ParseAllPatterns(patternfile, patternfile_js, &groups, &styles);
}

int PatternLoader::CloseFiles() {
  int r1 = true;
  int r2 = true;
  //if (patternfile) r1 = fclose(patternfile);
  //if (patternfile_js) r2 = fclose(patternfile_js);
  return r1 && r2;
}

PatternLoader::~PatternLoader() {
	FreeGroups(&groups);
	FreeStyles(&styles);
}

void PatternLoader::PrintStyles() {
	current_style = FirstStyle(&styles);
	while(current_style) {
		int i;
		printf((const char*)_T(" Style Name: %s\n"),Style_GetName(current_style));
		printf((const char*)_T("  Length: %i\n"),Style_GetLength(current_style));
		printf((const char *)_T("  Data:\n"));
		for(i=0;i<(int)Style_GetLength(current_style);i++) {
			if((i%4) == 0) {
				printf("   {");
			} else if ((i%4) == 2) {
				printf("}{");
			}
			printf(" %i",Style_GetData(current_style)[i]);

			if ((i%4) == 0 || (i%4) == 2) {
				printf(", ");
			} else if ((i%4) == 3) {
				printf("}\n");
			}
		}
		current_style = NextStyle(current_style);
		printf("\n");
	}
}

void PatternLoader::PrintSections() {
	current_group = FirstGroup(&groups);
	printf((const char*)_T("Group Data\n"));
	while(current_group) {
		printf((const char*)_T(" Group Name: %s\n"),Group_GetName(current_group));

		current_pattern = Group_GetPatterns(current_group);
		while(current_pattern) {
			printf((const char*)_T("  Pattern Name: %s\n"),Patt_GetName(current_pattern));
			printf((const char*)_T("   Style: %s\n"),Patt_GetStyle(current_pattern));
			printf((const char*)_T("   Data: %s\n"),Patt_GetData(current_pattern));
			printf((const char*)_T("   Height Ratio: %1.2f\n"),Patt_GetHR(current_pattern));
			printf((const char*)_T("   Dwell Ratio: %1.2f\n"),Patt_GetDR(current_pattern));
			printf((const char*)_T("   Author: %s\n"),Patt_GetAuthor(current_pattern));
			current_pattern = NextPatt(current_pattern);
		}

		current_group = NextGroup(current_group);
		printf("\n");
	}
}

JML_INT8* PatternLoader::GetStyle(const char *stylename) {
	if(current_style != NULL && strcmp(Style_GetName(current_style), stylename ) == 0) {
		/* Save iterating across stuff if we're already there */
		return (JML_INT8 *)Style_GetData(current_style);
	}

	current_style = FirstStyle(&styles);
	while(current_style) {
		if(strcmp(Style_GetName(current_style), stylename ) == 0) {
			return (JML_INT8 *)Style_GetData(current_style);
		}
		current_style = NextStyle(current_style);
	}
	return NULL;
}

JML_UINT8 PatternLoader::GetStyleLength(const char *stylename) {
	if(current_style != NULL && strcmp(Style_GetName(current_style), stylename ) == 0) {
		/* Save iterating across stuff if we're already there */
		return Style_GetLength(current_style);
	}

	current_style = FirstStyle(&styles);
	while(current_style) {
		if(strcmp(Style_GetName(current_style), stylename ) == 0) {
			return Style_GetLength(current_style);
		}
		current_style = NextStyle(current_style);
	}
	return 0;
}

const char *PatternLoader::GetFirstSection() {
	current_group = FirstGroup(&groups);
	current_pattern = NULL;
	if(current_group == NULL) {
		return NULL;
	}
	return Group_GetName(current_group);
}

const char *PatternLoader::GetNextSection() {
	current_group = NextGroup(current_group);
	current_pattern = NULL;
	if(current_group == NULL) {
		return NULL;
	}
	return Group_GetName(current_group);
}

int PatternLoader::SetSection(const char *section_name) {
	if(section_name == NULL || *section_name == '\0') return 0;
	current_group = FirstGroup(&groups);
	while(current_group) {
		if(Group_GetName(current_group) == NULL || *(const char *)Group_GetName(current_group) == '\0') continue;
		if(strcmp(section_name,(const char *)Group_GetName(current_group)) == 0 ) {
			current_pattern = NULL;
			return 1;
		}
		current_group = NextGroup(current_group);
	}
	return 0;
}

const char *PatternLoader::GetNextPatternName() {
	if(current_group == NULL) {
		return NULL;
	}
	if(current_pattern == NULL) {
		current_pattern = Group_GetPatterns(current_group);
		if(current_pattern == NULL) {
			return NULL;
		}
		return Patt_GetName(current_pattern);
	}

	current_pattern = NextPatt(current_pattern);
	if(current_pattern == NULL) {
		return NULL;
	}
	return Patt_GetName(current_pattern);
}

struct pattern_t *PatternLoader::GetPattern(const char *section_name,const char *pattern_name) {
	if(section_name == NULL || pattern_name == NULL) return NULL;
	if(SetSection(section_name)) {
		current_pattern = Group_GetPatterns(current_group);
		while(current_pattern) {
			if(strcmp(Patt_GetName(current_pattern),pattern_name) == 0) {
				return(current_pattern);
			}
			current_pattern = NextPatt(current_pattern);
		}
	}
	return(NULL);
}


#ifdef PATT_STANDALONE
class JMPatt:public wxApp {
	public:
		bool OnInit();
};
IMPLEMENT_APP(JMPatt)

bool JMPatt::OnInit() {
	PatternLoader *p;
	p = new PatternLoader(NULL, DEFAULT_SEMAPHOREFILE);
	if(argc==1 || strcmp(argv[1], "-q") != 0) {
		p->PrintStyles();
		p->PrintSections();
	}
	delete p;

	p = new PatternLoader(NULL, DEFAULT_PATTERNFILE);
	if(argc==1 || strcmp(argv[1], "-q") != 0) {
		p->PrintStyles();
		p->PrintSections();
	}
	delete p;

	return(0);
}
#endif
