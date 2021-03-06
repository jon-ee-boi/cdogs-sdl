/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2016, 2019 Cong Xu
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/
#include "editor_ui_color.h"

#include <cdogs/draw/drawtools.h>
#include <cdogs/font.h>

#include "editor_ui.h"
#include "editor_ui_common.h"


typedef struct
{
	color_t Color;
	char Text[COLOR_STR_BUF];
	void *Data;
	ColorPickerGetFunc GetFunc;
	ColorPickerChangeFunc ChangeFunc;
} ColorTextData;
static char *ColorPickerText(UIObject *o, void *data);
static bool ColorPickerSetFromText(void *data);
typedef struct
{
	color_t Color;
	struct vec2i SwatchSize;
	struct vec2i SwatchPad;
	void *Data;
	ColorPickerChangeFunc ChangeFunc;
} ColorPickerData;
static void ColorPickerUpdateText(UIObject *o, void *data);
static void ColorPickerChange(void *data, int d);
static void ColorPickerDrawSwatch(
	UIObject *o, GraphicsDevice *g, struct vec2i pos, void *data);
// Create a colour picker using the C-Dogs palette
UIObject *CreateColorPicker(
	const struct vec2i pos, void *data,
	ColorPickerGetFunc getFunc, ColorPickerChangeFunc changeFunc)
{
	UIObject *c = UIObjectCreate(UITYPE_CONTEXT_MENU, 0, pos, svec2i_zero());
	c->IsDynamicData = true;
	c->Data = data;

	// Create a text input area for the colour hex code
	UIObject *oText = UIObjectCreate(
		UITYPE_TEXTBOX, 0, svec2i_zero(), FontStrSize("#abcdef"));
	oText->u.Textbox.IsEditable = true;
	oText->u.Textbox.TextLinkFunc = ColorPickerText;
	oText->u.Textbox.MaxLen = sizeof ((ColorTextData *)0)->Text - 1;
	CSTRDUP(oText->u.Textbox.Hint, "(enter color hex value)");
	oText->IsDynamicData = true;
	CMALLOC(oText->Data, sizeof(ColorTextData));
	// Initialise colour and text
	((ColorTextData *)oText->Data)->Color = getFunc(data);
	ColorStr(((ColorTextData *)oText->Data)->Text, getFunc(data));
	((ColorTextData *)oText->Data)->Data = data;
	((ColorTextData *)oText->Data)->GetFunc = getFunc;
	((ColorTextData *)oText->Data)->ChangeFunc = changeFunc;
	oText->OnUnfocusFunc = ColorPickerSetFromText;
	// Constantly check the current mission colour and update text accordingly
	oText->CheckVisible = ColorPickerUpdateText;
	UIObjectAddChild(c, oText);

	const int yStart = oText->Size.y + 4;

	// Create colour squares from the palette
	const struct vec2i swatchSize = svec2i(5, 5);
	const struct vec2i swatchPad = svec2i(2, 2);
	UIObject *o = UIObjectCreate(
		UITYPE_CUSTOM, 0, svec2i_zero(), svec2i_add(swatchSize, swatchPad));
	o->ChangeFunc = ColorPickerChange;
	// Changing colour updates the masked pics for the mission, which requires
	// a reload
	o->ChangesData = true;
	o->ReloadData = true;
	o->u.CustomDrawFunc = ColorPickerDrawSwatch;
	const Pic *palette = PicManagerGetPic(&gPicManager, "palette");
	struct vec2i v;
	for (v.y = 0; v.y < palette->size.y; v.y++)
	{
		for (v.x = 0; v.x < palette->size.x; v.x++)
		{
			const color_t colour = PIXEL2COLOR(
				palette->Data[v.x + v.y * palette->size.x]);
			if (colour.a == 0)
			{
				continue;
			}
			UIObject *o2 = UIObjectCopy(o);
			o2->IsDynamicData = true;
			CMALLOC(o2->Data, sizeof(ColorPickerData));
			((ColorPickerData *)o2->Data)->Color = colour;
			((ColorPickerData *)o2->Data)->SwatchSize = swatchSize;
			((ColorPickerData *)o2->Data)->SwatchPad = swatchPad;
			((ColorPickerData *)o2->Data)->Data = data;
			((ColorPickerData *)o2->Data)->ChangeFunc = changeFunc;
			o2->Pos = svec2i_multiply(v, o->Size);
			o2->Pos.y += yStart;
			UIObjectAddChild(c, o2);
		}
	}
	UIObjectDestroy(o);

	// Create a dummy label that can be clicked to close the context menu
	CreateCloseLabel(c, svec2i(
		palette->size.x * (swatchSize.x + swatchPad.x) - FontStrW("Close"),
		0));

	return c;
}
static void ColorPickerUpdateText(UIObject *o, void *data)
{
	// Only update text if the text box doesn't have focus
	if (o->Parent->Highlighted == o) return;
	ColorTextData *cData = data;
	cData->Color = cData->GetFunc(cData->Data);
	ColorStr(cData->Text, cData->Color);
}
static char *ColorPickerText(UIObject *o, void *data)
{
	UNUSED(o);
	ColorTextData *cData = data;
	return cData->Text;
}
static bool ColorPickerSetFromText(void *data)
{
	ColorTextData *cData = data;
	const color_t colour = StrColor(cData->Text);
	cData->ChangeFunc(colour, cData->Data);
	return true;
}
static void ColorPickerChange(void *data, int d)
{
	UNUSED(d);
	ColorPickerData *mc = data;
	mc->ChangeFunc(mc->Color, mc->Data);
}
static void ColorPickerDrawSwatch(
	UIObject *o, GraphicsDevice *g, struct vec2i pos, void *data)
{
	UNUSED(o);
	const ColorPickerData *cpd = data;
	DrawRectangle(
		g,
		svec2i_add(svec2i_add(pos, o->Pos),
		svec2i_scale_divide(cpd->SwatchPad, 2)),
		cpd->SwatchSize,
		cpd->Color,
		true);
}


typedef enum
{
	MISSION_COLOR_WALL,
	MISSION_COLOR_FLOOR,
	MISSION_COLOR_ROOM,
	MISSION_COLOR_EXTRA,
	MISSION_COLOR_COUNT
} MissionColorType;
typedef struct
{
	CampaignOptions *C;
	MissionColorType Type;
} MissionColorData;
static const char *MissionGetColorStr(UIObject *o, void *data);
static void MissionColorChange(const color_t c, void *data);
static color_t CampaignGetMissionColor(void *data);
struct vec2i CreateColorObjs(CampaignOptions *co, UIObject *c, struct vec2i pos)
{
	const int th = FontH();

	UIObject *o = UIObjectCreate(
		UITYPE_LABEL, 0, svec2i_zero(), svec2i(100, th));
	o->ChangesData = true;
	o->ReloadData = true;
	o->u.LabelFunc = MissionGetColorStr;

	for (int i = 0; i < (int)MISSION_COLOR_COUNT; i++)
	{
		UIObject *o2 = UIObjectCopy(o);
		o2->IsDynamicData = true;
		CMALLOC(o2->Data, sizeof(MissionColorData));
		((MissionColorData *)o2->Data)->C = co;
		((MissionColorData *)o2->Data)->Type = (MissionColorType)i;
		o2->Pos = pos;

		MissionColorData *mcd;
		CMALLOC(mcd, sizeof *mcd);
		mcd->C = co;
		mcd->Type = (MissionColorType)i;
		UIObjectAddChild(o2, CreateColorPicker(
			svec2i_zero(), mcd, CampaignGetMissionColor, MissionColorChange));
		UIObjectAddChild(c, o2);
		pos.y += th;
	}

	UIObjectDestroy(o);
	return pos;
}
static const char *MissionGetColorStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	MissionColorData *mc = data;
	static const char *colourTypeNames[] =
	{
		"Walls", "Floors", "Rooms", "Extra"
	};
	char c[COLOR_STR_BUF];
	ColorStr(c, CampaignGetMissionColor(mc));
	sprintf(s, "%s: #%s", colourTypeNames[(int)mc->Type], c);
	return s;
}
static void MissionColorChange(const color_t c, void *data)
{
	MissionColorData *mcd = data;
	Mission *m = CampaignGetCurrentMission(mcd->C);
	if (m->Type == MAPTYPE_STATIC) return;
	MissionTileClasses *mtc =
		m->Type == MAPTYPE_CLASSIC ?
		&m->u.Classic.TileClasses : &m->u.Cave.TileClasses;
	switch (mcd->Type)
	{
	case MISSION_COLOR_WALL:
		mtc->Wall.Mask = c;
		mtc->Wall.Pic = TileClassGetPic(&gPicManager, &mtc->Wall);
		break;
	case MISSION_COLOR_FLOOR:
		mtc->Floor.Mask = c;
		mtc->Floor.Pic = TileClassGetPic(&gPicManager, &mtc->Floor);
		break;
	case MISSION_COLOR_ROOM:
		mtc->Room.Mask = c;
		mtc->Room.Pic = TileClassGetPic(&gPicManager, &mtc->Room);
		break;
	case MISSION_COLOR_EXTRA:
		mtc->Wall.MaskAlt = c;
		mtc->Floor.MaskAlt = c;
		mtc->Room.MaskAlt = c;
		mtc->Wall.Pic = TileClassGetPic(&gPicManager, &mtc->Wall);
		mtc->Floor.Pic = TileClassGetPic(&gPicManager, &mtc->Floor);
		mtc->Room.Pic = TileClassGetPic(&gPicManager, &mtc->Room);
		break;
	default:
		CASSERT(false, "Unexpected mission colour");
		break;
	}
}
static color_t CampaignGetMissionColor(void *data)
{
	MissionColorData *mcd = data;
	const Mission *m = CampaignGetCurrentMission(mcd->C);
	if (m == NULL) return colorWhite;
	if (m->Type == MAPTYPE_STATIC) return colorWhite;
	const MissionTileClasses *mtc =
		m->Type == MAPTYPE_CLASSIC ?
		&m->u.Classic.TileClasses : &m->u.Cave.TileClasses;
	switch (mcd->Type)
	{
	case MISSION_COLOR_WALL: return mtc->Wall.Mask;
	case MISSION_COLOR_FLOOR: return mtc->Floor.Mask;
	case MISSION_COLOR_ROOM: return mtc->Room.Mask;
	case MISSION_COLOR_EXTRA: return mtc->Wall.MaskAlt;
	default:
		CASSERT(false, "Unexpected mission colour");
		return colorWhite;
	}
}
