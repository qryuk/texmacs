
/******************************************************************************
* MODULE     : bitmap_ops.cpp
* DESCRIPTION: operation on character bitmaps
* COPYRIGHT  : (C) 1999  Joris van der Hoeven
*******************************************************************************
* This software falls under the GNU general public license and comes WITHOUT
* ANY WARRANTY WHATSOEVER. See the file $TEXMACS_PATH/LICENSE for more details.
* If you don't have this file, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
******************************************************************************/

#include "bitmap_font.hpp"
#include "ps_device.hpp"

glyph
join (glyph gl1, glyph gl2) {
  int x1= min (-gl1->xoff, -gl2->xoff);
  int y1= min (gl1->yoff- gl1->height, gl2->yoff- gl2->height);
  int x2= max (gl1->width- gl1->xoff, gl2->width- gl2->xoff);
  int y2= max (gl1->yoff, gl2->yoff);
  glyph bmr (x2-x1, y2-y1, -x1, y2, max (gl1->depth, gl2->depth));

  int i, j, dx, dy;
  dx= -gl1->xoff- x1, dy= y2- gl1->yoff;
  for (j=0; j<gl1->height; j++)
    for (i=0; i<gl1->width; i++)
      bmr->set_x (i+dx, j+dy, gl1->get_x (i, j));

  dx= -gl2->xoff- x1; dy= y2- gl2->yoff;
  for (j=0; j<gl2->height; j++)
    for (i=0; i<gl2->width; i++)
      bmr->set_x (i+dx, j+dy,
		  max (bmr->get_x (i+dx, j+dy), gl2->get_x (i, j)));

  return bmr;
}

glyph
move (glyph gl, SI x, SI y) {
  x += PIXEL/2; y += PIXEL/2; abs_round (x, y);
  int xx= x/PIXEL, yy= y/PIXEL;
  int ww= gl->width, hh= gl->height;
  glyph bmr (ww, hh, gl->xoff- xx, gl->yoff+ yy, gl->depth);

  int i, j;
  for (j=0; j<hh; j++)
    for (i=0; i<ww; i++)
      bmr->set_x (i, j, gl->get_x (i, j));
  bmr->lwidth= gl->lwidth;
  return bmr;
}

glyph
hor_flip (glyph gl) {
  int i, j;
  int ww= gl->width, hh= gl->height;
  glyph bmr (ww, hh, gl->xoff, gl->yoff, gl->depth);
  for (j=0; j<hh; j++)
    for (i=0; i<ww; i++)
      bmr->set_x (ww-1-i, j, gl->get_x (i, j));
  bmr->lwidth= gl->lwidth;
  return bmr;
}

glyph
ver_flip (glyph gl) {
  int i, j;
  int ww= gl->width, hh= gl->height;
  glyph bmr (ww, hh, gl->xoff, gl->yoff, gl->depth);
  for (j=0; j<hh; j++)
    for (i=0; i<ww; i++)
      bmr->set_x (i, hh-1-j, gl->get_x (i, j));
  bmr->lwidth= gl->lwidth;
  return bmr;
}

glyph
pos_rotate (glyph gl) {
  int i, j;
  int ww= gl->width, hh= gl->height;
  glyph bmr (hh, ww, gl->yoff, gl->width- gl->xoff, gl->depth);
  for (j=0; j<hh; j++)
    for (i=0; i<ww; i++)
      bmr->set_x (j, ww-1-i, gl->get_x (i, j));
  return bmr;
}

glyph
hor_extend (glyph gl, int pos, int by) {
  int i, j;
  int ww= gl->width, hh= gl->height;
  glyph bmr (ww+ by, hh, gl->xoff, gl->yoff, gl->depth);
  for (j=0; j<hh; j++)
    for (i=0; i<(ww+by); i++)
      bmr->set_x (i, j, gl->get_x (i<pos? i: (i<pos+by? pos: i-by), j));
  return bmr;
}

glyph
ver_extend (glyph gl, int pos, int by) {
  int i, j;
  int ww= gl->width, hh= gl->height;
  glyph bmr (ww, hh+by, gl->xoff, gl->yoff, gl->depth);
  for (j=0; j<(hh+by); j++)
    for (i=0; i<ww; i++)
      bmr->set_x (i, j, gl->get_x (i, j<pos? j: (j<pos+by? pos: j-by)));
  return bmr;
}
