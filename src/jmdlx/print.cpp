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

#include "print.h"
#ifndef __WXMSW__   
/*
Not needed under Windows
*/
    #include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>


enum {
	CHOOSEFILE
};

BEGIN_EVENT_TABLE(Print, wxDialog)
	EVT_BUTTON(wxID_OK, Print::OnOK)
	EVT_BUTTON(CHOOSEFILE, Print::OnChooseFile)
END_EVENT_TABLE()

Print::Print(wxWindow *parent, JMLib *j)
	: wxDialog(parent, -1, _T("Print"),
			wxDefaultPosition, wxDefaultSize,
			wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER) {

	jmlib = j;

	lastpath = wxGetHomeDir();

  // Filename
	wxBoxSizer *filenamesizer = new wxBoxSizer(wxHORIZONTAL);
	filename = new wxTextCtrl(this,-1,wxString(jmlib->getPattName(),wxConvUTF8));
	filenamesizer->Add(new wxStaticText(this, 0, _T("Filename")),
                                        0,
                                        wxALIGN_CENTER_VERTICAL|wxALL,
                                        5);
	filenamesizer->Add(filename,
                        1,
                        wxALIGN_CENTRE_VERTICAL|wxALL,
                        5);

	filenamesizer->Add(new wxButton(this, CHOOSEFILE, _T("Choose File")),
                        1,
                        wxALIGN_CENTRE_VERTICAL|wxALL,
                        5);

  // Output Type
	wxBoxSizer *typesizer = new wxBoxSizer(wxHORIZONTAL);
	output_type = new wxChoice(this,-1);
	output_type->Append(_T("Image"));
	output_type->Append(_T("PostScript"));
	output_type->SetStringSelection(_T("PostScript"));
	output_type->Append(_T("FlipBook"));

#ifdef HAVE_AVCODEC_H
	output_type->Append(_T("MPEG"));
	output_type->SetStringSelection(_T("MPEG"));
#endif

	typesizer->Add(new wxStaticText(this, 0, _T("Output Type")),
                                        0,
                                        wxALIGN_CENTER_VERTICAL|wxALL,
                                        5);
	typesizer->Add(output_type,
                        1,
                        wxALIGN_CENTRE_VERTICAL|wxALL,
                        5);

  // Width, Height, Delay, Max Frames
	wxFlexGridSizer *whdm = new wxFlexGridSizer(2,5,5);

	handed = new wxChoice(this,-1);
	handed->Append(_T("Left"));
	handed->Append(_T("Right"));
	handed->SetStringSelection(_T("Left"));

	delay = new wxSpinCtrl(this,
				-1,
				wxString::Format(_T("%d"), 30),
				wxDefaultPosition,
				wxDefaultSize,
				wxSP_ARROW_KEYS,
				1,
				1000,
				30);

	max_iterations = new wxSpinCtrl(this,
				-1,
				wxString::Format(_T("%d"), 200),
				wxDefaultPosition,
				wxDefaultSize,
				wxSP_ARROW_KEYS,
				1,
				10000,
				1000);

	whdm->Add(new wxStaticText(this, 0, _T("Delay")),
				1, wxALIGN_RIGHT|wxALIGN_CENTRE_VERTICAL|wxALL, 5);
	whdm->Add(delay,
				1, wxALIGN_LEFT|wxALL, 5);
	whdm->Add(new wxStaticText(this, 0, _T("Max Frames")),
				1, wxALIGN_RIGHT|wxALIGN_CENTRE_VERTICAL|wxALL, 5);
	whdm->Add(max_iterations,
				1, wxALIGN_LEFT|wxALL, 5);
	whdm->Add(new wxStaticText(this, 0, _T("FlipBook Handedness")),
				1, wxALIGN_RIGHT|wxALIGN_CENTRE_VERTICAL|wxALL, 5);
	whdm->Add(handed,
				1, wxALIGN_LEFT|wxALL, 5);

  // Width, Height, Delay, Max Frames
	wxButton *ok = new wxButton(this, wxID_OK, _T("OK"));
	wxButton *cancel = new wxButton(this, wxID_CANCEL, _T("Cancel"));
	wxBoxSizer *buttonsizer = new wxBoxSizer(wxHORIZONTAL);
	buttonsizer->Add(ok, 1, wxALIGN_CENTRE|wxALL, 5);
	buttonsizer->Add(cancel, 1, wxALIGN_CENTRE|wxALL, 5);

	wxBoxSizer *toplevel = new wxBoxSizer(wxVERTICAL);
	toplevel->Add(filenamesizer,0,wxALIGN_CENTER|wxEXPAND|wxALL,5);
	toplevel->Add(typesizer,0,wxALIGN_CENTER|wxEXPAND|wxALL,5);
	toplevel->Add(whdm,0,wxALIGN_CENTER|wxEXPAND|wxALL,5);
	toplevel->Add(buttonsizer,0,wxALIGN_CENTER|wxEXPAND|wxALL,5);

	toplevel->Fit( this );
	toplevel->SetSizeHints( this );

	SetSizer(toplevel);
	SetAutoLayout(TRUE);
	Layout();
	CentreOnParent();
	ShowModal();

}

void Print::OnOK(wxCommandEvent &WXUNUSED(event)) {
	int oldheight = jmlib->getImageHeight();
	int oldwidth = jmlib->getImageWidth();
	int do_change = 0;
	int print_success = 0;
	int i;
	struct stat buf; /* for stat */
	wxMessageDialog* message;

	if(stat((const char *)filename->GetValue().mb_str(wxConvUTF8),&buf) != -1) {
		message = new wxMessageDialog(this,
			_T("File Already Exists! Overwrite?"),
			_T("Overwrite?"),
			wxYES_NO|wxICON_EXCLAMATION);
		if(message->ShowModal() != wxID_YES) {
			delete outputfile;
			return;
		}
		wxRemoveFile(filename->GetValue());
	}


	if (output_type->GetStringSelection() == _T("Image")) {
		print_success = printImage();
	}

	if (output_type->GetStringSelection() == _T("PostScript")) {
		print_success = printPS();
	}

	if (output_type->GetStringSelection() == _T("FlipBook")) {
		print_success = printFlipBook();
	}

#ifdef HAVE_AVCODEC_H
	if (output_type->GetStringSelection() == _T("MPEG")) {
		print_success = printMPEG();
	}
#endif

	if(print_success != 0) {
		wxMessageDialog message(this,
				_T("Printing Aborted!"), _T("Aborted"),
				wxOK|wxICON_EXCLAMATION);
		message.ShowModal();
		wxRemoveFile(filename->GetValue());
	}

	if(do_change) {
		jmlib->setWindowSize(oldwidth, 
			oldheight);

		/* Just make sure it clears out any guff */
		for (i=0; i<200; i++) jmlib->doJuggle();
	}
	/* Just make sure it clears out any guff */
	for (i=0; i<200; i++) jmlib->doJuggle();
	EndModal(wxID_OK);
}

void Print::OnChooseFile(wxCommandEvent &WXUNUSED(event)) {
	wxFileDialog filedialog(this, _T("Choose a File to Print to"),
		lastpath, wxT(""),
		_T("All Files|*"),
		wxSAVE);

	if(filedialog.ShowModal() != wxID_OK) return;

	lastpath = filedialog.GetDirectory();

	filename->SetValue(filedialog.GetPath());
}

WX_DECLARE_LIST(wxImageHandler, HandlerList);
#include <wx/listimpl.cpp>
WX_DEFINE_LIST(HandlerList);


int Print::printImage() {
	wxDialog formatchooser(this, -1, _T("Choose Format"),
			wxDefaultPosition, wxDefaultSize,
			wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);

	wxChoice *formatchoice = new wxChoice( &formatchooser, -1 );
	
	wxInitAllImageHandlers();

	wxString fileextn;
	fileextn = filename->GetValue().AfterLast('.');
	int formatfound = 0;

	wxImageHandler* handler;
	HandlerList& handlers = (HandlerList&)wxImage::GetHandlers();
	// wxList handlers = wxImage::GetHandlers();
	// ArrayOfIH handlers = (ArrayOfIH&)wxImage::GetHandlers();
	HandlerList::Node *node = handlers.GetFirst();
	wxString extn;
	while(node) {
		handler = (wxImageHandler *)node->GetData();
		extn = handler->GetExtension();
		/* Don't append if wx doesn't support writing */
		if(extn.Len() > 0 &&
			extn != wxT("cur") && /* Silly format */
			extn != wxT("ico") && /* Silly format */
			extn != wxT("iff") && /* wx Doesn't support writing */
			extn != wxT("gif") && /* wx Doesn't support writing */
			extn != wxT("ani") /* wx Doesn't support writing */
			) {
			formatchoice->Append(handler->GetExtension(), (void *)handler);
			if(extn == fileextn) {
				formatchoice->SetSelection(formatchoice->FindString(handler->GetExtension()));
				formatfound++;
			}
		}
		node = node->GetNext();
	}

	if(formatfound == 0) {
		int png_pos = formatchoice->FindString(wxT("png"));
		if(-1 != png_pos) formatchoice->SetSelection(png_pos);
		else formatchoice->SetSelection(0);
	}


	wxButton *ok = new wxButton(&formatchooser, wxID_OK, _T("OK"));
	wxButton *cancel = new wxButton(&formatchooser, wxID_CANCEL, _T("Cancel"));
	wxBoxSizer *buttonsizer = new wxBoxSizer(wxHORIZONTAL);
	buttonsizer->Add(ok, 1, wxALIGN_CENTRE|wxALL, 5);
	buttonsizer->Add(cancel, 1, wxALIGN_CENTRE|wxALL, 5);

	wxBoxSizer *toplevel = new wxBoxSizer(wxVERTICAL);
	toplevel->Add(formatchoice,1,wxALIGN_CENTER|wxEXPAND|wxALL,5);
	toplevel->Add(buttonsizer,0,wxALIGN_CENTER|wxEXPAND|wxALL,5);

	toplevel->Fit( &formatchooser );
	toplevel->SetSizeHints( &formatchooser );

	formatchooser.SetSizer(toplevel);
	formatchooser.SetAutoLayout(TRUE);
	formatchooser.Layout();
	formatchooser.CentreOnParent();
	if(formatchooser.ShowModal() != wxID_OK) {
		return 1;
	}

	wxBitmap frame(jmlib->getImageWidth(), jmlib->getImageHeight());
	wxImage image;
	wxMemoryDC dc;


	dc.SelectObject(frame);

	RenderFrame(&dc, jmlib);

	image = frame.ConvertToImage();

	const wxString type = ((wxImageHandler *)formatchoice->GetClientData(
			formatchoice->GetSelection()))->GetMimeType();

	if(image.SaveFile(filename->GetValue(), type ) == TRUE) {
		return 0;
	}
	return 1;
}

int Print::printPS(void) {


	/* Note that postscript's co-ordinate system is upside down,
		c.f. jmlib and wx */

	int y_offset;
	struct ball firstpos[BMAX]; /* We'll rememeber where all the
				balls were when we started, and check
				against it */

	wxProgressDialog progress(_T("Progress"),_T("Creating PostScript"),
		max_iterations->GetValue(), this,
		wxPD_APP_MODAL|wxPD_CAN_ABORT);


	y_offset=jmlib->imageHeight;

	int i;
	int current_frames = 0;
	int done = 0;
	arm* ap = &(jmlib->ap);
	ball* rhand = &(jmlib->rhand);
	ball* lhand = &(jmlib->lhand);
	hand* handp = &(jmlib->handpoly);

	outputfile = fopen((const char *)filename->GetValue().mb_str(wxConvUTF8),"w");
	if(outputfile == NULL) return 1;

	/* Some PS guff */

	fprintf(outputfile, "%%%%!PS-Adobe-3.0\n");
	fprintf(outputfile, "%%%%BoundingBox: 0 0 %i %i\n",
				jmlib->getImageWidth(),
				jmlib->getImageHeight());
	fprintf(outputfile, "%%%%Creator: JuggleMaster And Chunky Kibbles\n");
	fprintf(outputfile, "%%%%Title: Juggling Pattern %s\n", jmlib->getSite());
	fprintf(outputfile, "%%%%EndComments\n");

	/* The start of stuff for dynamically resizing to fit the page
		instead of using a bounding box */

	fprintf(outputfile,"initclip newpath clippath pathbbox\n");
	fprintf(outputfile,"72 div /pageTop exch def\n");
	fprintf(outputfile,"72 div /pageRight exch def\n");
	fprintf(outputfile,"72 div /pageBottom exch def\n");
	fprintf(outputfile,"72 div /pageLeft exch def\n");
	fprintf(outputfile,"/pageWidth pageRight pageLeft sub def\n");
	fprintf(outputfile,"/pageHeight pageTop pageBottom sub def\n");
	fprintf(outputfile,"72 72 dtransform\n");
	fprintf(outputfile,"/yResolution exch abs def\n");
	fprintf(outputfile,"/xResolution exch abs def\n");

	fprintf(outputfile,"/Times-Roman findfont\n");
	fprintf(outputfile,"15 scalefont\n");
	fprintf(outputfile,"setfont\n");

	fprintf(outputfile, "%i {\n", max_iterations->GetValue());

	for(i=jmlib->numBalls()-1;i>=0;i--) {
		firstpos[i] = jmlib->b[i];
	}

	while (!done) {
		jmlib->doJuggle();

		done = 1;
		for(i=jmlib->numBalls()-1;i>=0;i--) {
			if(firstpos[i].gx != jmlib->b[i].gx ||
				firstpos[i].gy != jmlib->b[i].gy) done=0;
		}
		current_frames++;
		if(current_frames > max_iterations->GetValue()) done=1;

		fprintf(outputfile, "erasepage\n");
		fprintf(outputfile, "2 2 moveto\n"	
				"( Site: %s  Style: %s  Balls: %i ) show\n",
				jmlib->getSite(),
				jmlib->getStyle(),
				jmlib->numBalls());

		fprintf(outputfile, "%i {\n", delay->GetValue());
		/* Draw Juggler */

		/* Head */

		fprintf(outputfile,"%%Head\n");
		fprintf(outputfile,"newpath\n");
		fprintf(outputfile, " %i %i %i 0 360 arc\n", ap->hx,
							-ap->hy + y_offset,
							ap->hr);
		fprintf(outputfile, " closepath\n");
		fprintf(outputfile, " stroke\n");


		/* Right Arm */

		fprintf(outputfile, "%%Right Arm\n");
		fprintf(outputfile, "newpath\n");
		fprintf(outputfile, " %i %i moveto\n", ap->rx[0],
				-ap->ry[0] + y_offset);
		for(i=1;i<6;i++){
			fprintf(outputfile, "  %i %i lineto\n", ap->rx[i],
					-ap->ry[i] + y_offset);
		}
		fprintf(outputfile, " stroke\n");


		/* Left Arm */

		fprintf(outputfile, "%%Left Arm\n");
		fprintf(outputfile, "newpath\n");
		fprintf(outputfile, " %i %i moveto\n", ap->lx[0],
					-ap->ly[0] + y_offset);
		for(i=1;i<6;i++){
			fprintf(outputfile, "  %i %i lineto\n", ap->lx[i],
						-ap->ly[i] + y_offset);
		}
		fprintf(outputfile, " stroke\n");


		/* Right Hand */

		fprintf(outputfile, "%%Right Hand\n");
		fprintf(outputfile, "newpath\n");
		fprintf(outputfile, " %i %i moveto\n", rhand->gx + handp->rx[0],
				-(rhand->gy + handp->ry[0])+y_offset);
		for (i=1; i <= 9; i++) {
			fprintf(outputfile, "  %i %i lineto\n",
				rhand->gx + handp->rx[i],
				-(rhand->gy + handp->ry[i])+y_offset);
		}
		fprintf(outputfile, " closepath\n");
		fprintf(outputfile, " stroke\n");


		/* Left Hand */
	
		fprintf(outputfile, "%%Left Hand\n");
		fprintf(outputfile, "newpath\n");
		fprintf(outputfile, " %i %i moveto\n", lhand->gx + handp->lx[0],
				-(lhand->gy + handp->ly[0])+y_offset);
		for (i=1; i <= 9; i++) {
			fprintf(outputfile, "  %i %i lineto\n",
				lhand->gx + handp->lx[i],
				-(lhand->gy + handp->ly[i])+y_offset);
		}
		fprintf(outputfile, " closepath\n");
		fprintf(outputfile, " stroke\n");


		/* Balls */

		fprintf(outputfile, "%%Balls\n");
		fprintf(outputfile, "newpath\n");
		int diam = 11*jmlib->dpm/DW;
		for(i=jmlib->numBalls()-1;i>=0;i--) {
			fprintf(outputfile, " %i %i %i 0 360 arc\n",
				jmlib->b[i].gx+diam,
				-jmlib->b[i].gy-diam + y_offset,
				diam);
			fprintf(outputfile, " fill\n");
		}
		fprintf(outputfile, " stroke\n");


		fprintf(outputfile, "} repeat\n");

		if(current_frames % 10 == 0) {
			if(FALSE == progress.Update(current_frames)) {
				return 1;
			}
		}
	}

	fprintf(outputfile, "} repeat\n");

	fclose(outputfile);
	return 0;

}

int Print::printFlipBook(void) {


	/* Note that postscript's co-ordinate system is upside down,
		c.f. jmlib and wx */

	int y_offset;
	int current_page = 1;
	struct ball firstpos[BMAX]; /* We'll rememeber where all the
				balls were when we started, and check
				against it */

	int flipbook_width = 3;
	int flipbook_height = 5;
	int frame_width;
	int frame_height;

	wxProgressDialog progress(_T("Progress"),_T("Creating FlipBook"),
		max_iterations->GetValue(), this,
		wxPD_APP_MODAL|wxPD_CAN_ABORT);


	y_offset=jmlib->imageHeight;

	int i, j, k;
	int current_frames = 0;
	int done = 0;
	arm* ap = &(jmlib->ap);
	ball* rhand = &(jmlib->rhand);
	ball* lhand = &(jmlib->lhand);
	hand* handp = &(jmlib->handpoly);

	outputfile = fopen((const char *)filename->GetValue().mb_str(wxConvUTF8),"w");
	if(outputfile == NULL) return 1;


	/* Make the space at the side of each flip [for the staple]
	1/4 of the width of the image itself. */
	frame_width = jmlib->getImageWidth() + (jmlib->getImageWidth()/4);
	frame_height = jmlib->getImageHeight();

	/* Some PS guff */
	fprintf(outputfile, "%%!PS-Adobe-3.0\n");
	fprintf(outputfile, "%%%%BoundingBox: 0 0 612 792\n");
	fprintf(outputfile, "%%%%Creator: JuggleMaster And Chunky Kibbles\n");
	fprintf(outputfile, "%%%%Title: Juggling Pattern %s\n", jmlib->getSite());
	fprintf(outputfile, "%%%%Pages: (atend)\n");
	fprintf(outputfile, "%%%%Orientation: Portrait\n");
	fprintf(outputfile, "%%%%LanguageLevel: 1\n");
	fprintf(outputfile, "%%%%DocumentData: Clean7Bit\n");
	fprintf(outputfile, "%%%%PageOrder: Ascend\n");
	fprintf(outputfile, "%%%%EndComments\n");

	/* The start of stuff for dynamically resizing to fit the page
		instead of using a bounding box */

	fprintf(outputfile, "%%%%BeginProlog\n");
	fprintf(outputfile,
		"/initialisePage {\n"
		"  clear\n"
		"  clippath pathbbox\n"
		"  72 div /pageTop exch def\n"
		"  72 div /pageRight exch def\n"
		"  72 div /pageBottom exch def\n"
		"  72 div /pageLeft exch def\n"
		"  /pageWidth pageRight pageLeft sub def\n"
		"  /pageHeight pageTop pageBottom sub def\n"
		"  72 72 dtransform\n"
		"  /yResolution exch abs def\n"
		"  /xResolution exch abs def\n"

		"  612 %i div 792 %i div scale\n"
		"  /Times-Roman findfont\n"
		"  15 scalefont\n"
		"  setfont\n"
		"} def\n",
		frame_width * flipbook_width, frame_height * flipbook_height);

	fprintf(outputfile, "%%%%EndProlog\n");

	for(i=jmlib->numBalls()-1;i>=0;i--) {
		firstpos[i] = jmlib->b[i];
	}

	while (!done) {

		done = 1;
		jmlib->doJuggle();
		for(i=jmlib->numBalls()-1;i>=0;i--) {
			if(firstpos[i].gx != jmlib->b[i].gx ||
				firstpos[i].gy != jmlib->b[i].gy) done=0;
		}

		fprintf(outputfile, "%%%%Page: %i %i\n", current_page, current_page);
		fprintf(outputfile, "gsave\n");
		fprintf(outputfile, "initialisePage\n");

		for(k=flipbook_height-1; k>=0; k--) {
			for(j=0; j<flipbook_width; j++) {
				int f_offs_x = j * frame_width;
				int f_offs_y = k * frame_height;

				/* Cut marks */
				fprintf(outputfile, "%i %i 2 2 rectfill\n",
					f_offs_x + 1, f_offs_y);
				fprintf(outputfile, "%i %i 2 2 rectfill\n",
					f_offs_x + frame_width - 1, f_offs_y);
				fprintf(outputfile, "%i %i 2 2 rectfill\n",
					f_offs_x + 1, f_offs_y + frame_height - 1);
				fprintf(outputfile, "%i %i 2 2 rectfill\n",
					f_offs_x + frame_width - 1, f_offs_y + frame_height - 1);


				current_frames++;
				jmlib->doJuggle();

				fprintf(outputfile, "%i %i moveto\n"	
					"( %i ) show\n",
					(handed->GetStringSelection() == wxT("Right")?
						f_offs_x + 20:
						f_offs_x + frame_width - 20),
					f_offs_y + frame_height/2,
					current_frames);


				if (handed->GetStringSelection() == wxT("Right")) {
					f_offs_x += (jmlib->getImageWidth()/4);
				}

				fprintf(outputfile, "%i %i moveto\n"	
					"( Site: %s  Style: %s  Balls: %i ) show\n",
					f_offs_x + 4, f_offs_y + 4,
					jmlib->getSite(),
					jmlib->getStyle(),
					jmlib->numBalls());

				/* Draw Juggler */


				/* Head */

				fprintf(outputfile,"%%Head\n");
				fprintf(outputfile,"newpath\n");
				fprintf(outputfile, " %i %i %i 0 360 arc\n", f_offs_x + ap->hx,
					f_offs_y + -ap->hy + y_offset, ap->hr);
				fprintf(outputfile, " closepath\n");
				fprintf(outputfile, " stroke\n");


				/* Right Arm */

				fprintf(outputfile, "%%Right Arm\n");
				fprintf(outputfile, "newpath\n");
				fprintf(outputfile, " %i %i moveto\n",
					f_offs_x + ap->rx[0], f_offs_y - ap->ry[0] + y_offset);
				for(i=1;i<6;i++){
					fprintf(outputfile, "  %i %i lineto\n",
						f_offs_x + ap->rx[i], f_offs_y - ap->ry[i] + y_offset);
				}
				fprintf(outputfile, " stroke\n");


				/* Left Arm */

				fprintf(outputfile, "%%Left Arm\n");
				fprintf(outputfile, "newpath\n");
				fprintf(outputfile, " %i %i moveto\n",
					f_offs_x + ap->lx[0], f_offs_y - ap->ly[0] + y_offset);
				for(i=1;i<6;i++){
					fprintf(outputfile, "  %i %i lineto\n",
						f_offs_x + ap->lx[i], f_offs_y - ap->ly[i] + y_offset);
				}
				fprintf(outputfile, " stroke\n");


				/* Right Hand */

				fprintf(outputfile, "%%Right Hand\n");
				fprintf(outputfile, "newpath\n");
				fprintf(outputfile, " %i %i moveto\n",
						f_offs_x + rhand->gx + handp->rx[0],
						f_offs_y - (rhand->gy + handp->ry[0])+y_offset);
				for (i=1; i <= 9; i++) {
					fprintf(outputfile, "  %i %i lineto\n",
						f_offs_x + rhand->gx + handp->rx[i],
						f_offs_y - (rhand->gy + handp->ry[i])+y_offset);
				}
				fprintf(outputfile, " closepath\n");
				fprintf(outputfile, " stroke\n");


				/* Left Hand */
	
				fprintf(outputfile, "%%Left Hand\n");
				fprintf(outputfile, "newpath\n");
				fprintf(outputfile, " %i %i moveto\n",
					f_offs_x + lhand->gx + handp->lx[0],
					f_offs_y -(lhand->gy + handp->ly[0])+y_offset);
				for (i=1; i <= 9; i++) {
					fprintf(outputfile, "  %i %i lineto\n",
						f_offs_x + lhand->gx + handp->lx[i],
						f_offs_y - (lhand->gy + handp->ly[i])+y_offset);
				}
				fprintf(outputfile, " closepath\n");
				fprintf(outputfile, " stroke\n");


				/* Balls */

				fprintf(outputfile, "%%Balls\n");
				fprintf(outputfile, "newpath\n");
				int diam = 11*jmlib->dpm/DW;
				for(i=jmlib->numBalls()-1;i>=0;i--) {
					fprintf(outputfile, " %i %i %i 0 360 arc\n",
						f_offs_x + jmlib->b[i].gx+diam,
						f_offs_y -jmlib->b[i].gy-diam + y_offset,
						diam);
					fprintf(outputfile, " fill\n");
				}
				fprintf(outputfile, " stroke\n");


				if(current_frames >= max_iterations->GetValue()) done=1;
				if(current_frames % 10 == 0 && !done) {
					if(FALSE == progress.Update(current_frames)) {
						fclose(outputfile);
						return 1;
					}
				}
			}
		}
		fprintf(outputfile, "grestore\n");
		fprintf(outputfile, "showpage\n");
		fprintf(outputfile, "%%%%EndPage: %i %i\n", current_page, current_page);
		current_page++;
	}

	fprintf(outputfile, "%%%%Trailer\n");
	fprintf(outputfile, "%%Frames: %i\n", current_frames);
	fprintf(outputfile, "%%%%Pages: %i\n", current_page);
	fprintf(outputfile, "%%%%EOF\n");
	fclose(outputfile);
	return 0;

}


#ifdef HAVE_AVCODEC_H
/* Pretty much taken from apiexample.c in ffmpeg/libavcodec */
int Print::printMPEG() {
	AVCodec *codec;
	AVCodecContext *c= NULL;
	AVFrame *picture;

	int i, out_size, size, outbuf_size;
	uint8_t *outbuf, *picture_buf;

	int x,y;

	wxBitmap frame(jmlib->getImageWidth(), jmlib->getImageHeight());
	wxImage image;
	wxMemoryDC dc;
	struct ball firstpos[BMAX];

	wxProgressDialog progress(_T("Progress"),_T("Creating MPEG"),
		max_iterations->GetValue(), this, wxPD_APP_MODAL|wxPD_CAN_ABORT);

	int current_frames = 0;
	int done = 0;

	outputfile = fopen((const char *)filename->GetValue(),"w");
	if(outputfile == NULL) return 1;

	avcodec_init();
	avcodec_register_all();

	codec = avcodec_find_encoder(CODEC_ID_MPEG1VIDEO);
	if (!codec) {
		return(1);
	}

	c= avcodec_alloc_context();
	picture= avcodec_alloc_frame();

	c->bit_rate = 400000;
	c->width = jmlib->getImageWidth();
	c->height = jmlib->getImageHeight();
	/* c->frame_rate = 70/delay->GetValue(); */
	c->frame_rate = 25;
	c->frame_rate_base= 1;
	c->gop_size = 30;
	c->max_b_frames=1;

	if (avcodec_open(c, codec) < 0) {
		return(1);
	}

	outbuf_size = 100000;
	outbuf = (uint8_t *)malloc(outbuf_size);
	size = c->width * c->height;
	picture_buf = (uint8_t *)malloc((size * 3) / 2); /* size for YUV 420 */

	picture->data[0] = picture_buf;
	picture->data[1] = picture->data[0] + size;
	picture->data[2] = picture->data[1] + size / 4;
	picture->linesize[0] = c->width;
	picture->linesize[1] = c->width / 2;
	picture->linesize[2] = c->width / 2;


	for(i=jmlib->numBalls()-1;i>=0;i--) {
		firstpos[i] = jmlib->b[i];
	}

	while (!done) {
		jmlib->doJuggle();

		done = 1;

		memset((void *)picture_buf, '\0', (size*3)/2);

		for(i=jmlib->numBalls()-1;i>=0;i--) {
			if(firstpos[i].gx != jmlib->b[i].gx ||
				firstpos[i].gy != jmlib->b[i].gy) done=0;
		}
		current_frames++;
		if(current_frames > max_iterations->GetValue()) done=1;

		dc.SelectObject(frame);

		RenderFrame(&dc, jmlib);

		image = frame.ConvertToImage();
		unsigned char why,cr,cb;
		for(x=0;x<c->width;x++) {
			for(y=0;y<c->height;y++) {
				why = RGBgetY(image.GetRed(x,y),image.GetGreen(x,y),image.GetBlue(x,y));
					picture->data[0][y*picture->linesize[0] + x] = why;
			}
		}
		for(x=0;x<c->width/2;x++) {
			for(y=0;y<c->height/2;y++) {
				cr = RGBgetCr(image.GetRed(x*2,y*2),image.GetGreen(x*2,y*2),image.GetBlue(x*2,y*2));
				cb = RGBgetCb(image.GetRed(x*2,y*2),image.GetGreen(x*2,y*2),image.GetBlue(x*2,y*2));
				picture->data[1][y*picture->linesize[1] + x] = cr;
				picture->data[2][y*picture->linesize[2] + x] = cb;
			}
		}

		out_size = avcodec_encode_video(c, outbuf, outbuf_size, picture);
		fwrite(outbuf, 1, out_size, outputfile);

		if(current_frames % 10 == 0) {
			if(FALSE == progress.Update(current_frames)) {
				return 1;
			}
		}
	}

	for(; out_size; i++) {
		out_size = avcodec_encode_video(c, outbuf, outbuf_size, NULL);
		fwrite(outbuf, 1, out_size, outputfile);
	}

	outbuf[0] = 0x00;
	outbuf[1] = 0x00;
	outbuf[2] = 0x01;
	outbuf[3] = 0xb7;
	fwrite(outbuf, 1, 4, outputfile);
	free(picture_buf);
	free(outbuf);

	avcodec_close(c);
	free(c);
	free(picture);

	fclose(outputfile);
	return(0);

}

/*
RGB -> YCbCr Conversions:

Y = (77R + 150G + 29B) / 256
Cr = (131R - 110G - 21B) / 256 + 128
Cb = (-44R - 87G + 131B) / 256 + 128
"Y ranges from 16 to 235, Cb and Cr range from 16 to 240"

*/

unsigned char Print::RGBgetY(unsigned char r, unsigned char g, unsigned char b) {
	unsigned char value;
	value = (77*r + 150*g + 29*b)/256;
	if(value > 235) value = 235;
	else if(value < 16) value = 16;
	return value;
	// return (unsigned char)((((77*r + 150*g + 29*b)/256)*235/256)+16);
	//return (unsigned char)(77*r + 150*g + 29*b)/256;
}

unsigned char Print::RGBgetCb(unsigned char r, unsigned char g, unsigned char b) {
	unsigned char value;
	value = 128 + (131*r - 110*g - 12*b)/256;
	if(value > 240) value = 240;
	else if(value < 16) value = 16;
	return value;
	// return (unsigned char)(((128 + (131*r - 110*g - 12*b)/256)*240/256)+16);
}

unsigned char Print::RGBgetCr(unsigned char r, unsigned char g, unsigned char b) {
	unsigned char value;
	value = 128 + (-44*r - 87*g + 131*b)/256;
	if(value > 240) value = 240;
	else if(value < 16) value = 16;
	return value;
	// return (unsigned char)(((128 + (-44*r - 87*g + 131*b)/256)*240/256)+16);
}

#endif

void Print::RenderFrame(wxDC *dc, JMLib *j) {
	int i;
	arm* ap = &(j->ap);
	ball* rhand = &(j->rhand);
	ball* lhand = &(j->lhand);
	hand* handp = &(j->handpoly);
	dc->SetBackground(*wxWHITE_BRUSH);
	dc->SetPen(*wxBLACK_PEN);
	dc->SetBrush(*wxWHITE_BRUSH);
	dc->Clear();
	// draw head
	dc->DrawEllipse(ap->hx - ap->hr, ap->hy - ap->hr, ap->hr*2, ap->hr*2);

	// draw juggler
	for(i=0;i<5;i++){
		dc->DrawLine(ap->rx[i], ap->ry[i], ap->rx[i+1], ap->ry[i+1]);
		dc->DrawLine(ap->lx[i], ap->ly[i], ap->lx[i+1], ap->ly[i+1]);
	}

	// hands
	for (i=0; i <= 8; i++) {
		dc->DrawLine(rhand->gx + handp->rx[i], rhand->gy + handp->ry[i],
			rhand->gx + handp->rx[i+1], rhand->gy + handp->ry[i+1]);
		dc->DrawLine(lhand->gx + handp->lx[i], lhand->gy + handp->ly[i],
			lhand->gx + handp->lx[i+1], lhand->gy + handp->ly[i+1]);
	}

	dc->DrawLine(rhand->gx + handp->rx[9], rhand->gy + handp->ry[9],
		rhand->gx + handp->rx[0], rhand->gy + handp->ry[0]);
	dc->DrawLine(lhand->gx + handp->lx[9], lhand->gy + handp->ly[9],
		lhand->gx + handp->lx[0], lhand->gy + handp->ly[0]);

	dc->SetBrush(*wxRED_BRUSH);

	// draw balls
	int diam = (11*j->dpm/DW)*2;
	for(i=j->numBalls()-1;i>=0;i--) {
		dc->DrawEllipse(j->b[i].gx, j->b[i].gy, diam, diam);
	}
	wxString balltext;
	balltext.Printf(_T("Site: %s    Style: %s    Balls: %i"),j->getSite(),j->getStyle(),j->numBalls());
	dc->DrawText(balltext, 10, 10);

}
