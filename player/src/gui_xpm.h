/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * gui_xpm.h - provides bitmaps for buttons for gtk gui
 */
static char *xpm_play[] = {
  "16 16 2 1", // 16x16, 2 colors, 1 char/color
  "  c None",
  "B c #000000000000",
  "                ", // 1
  "                ", // 2
  "   B            ", // 3
  "   BBB          ", // 4
  "   BBBBB        ", // 5
  "   BBBBBBB      ", // 6
  "   BBBBBBBBB    ", // 7
  "   BBBBBBBBBBB  ", // 8
  "   BBBBBBBBBBB  ", // 9
  "   BBBBBBBBB    ", // 10
  "   BBBBBBB      ", // 11
  "   BBBBB        ", // 12
  "   BBB          ", // 13
  "   B            ", // 14
  "                ", // 15
  "                ", // 16
};

static char *xpm_pause[] = {
  "16 16 2 1", // 16x16, 2 colors, 1 char/color
  "  c None",
  "B c #000000000000",
  "                ", // 1
  "                ", // 2
  "                ", // 3
  "                ", // 4
  "     BB   BB    ", // 5
  "     BB   BB    ", // 6
  "     BB   BB    ", // 7
  "     BB   BB    ", // 8
  "     BB   BB    ", // 9
  "     BB   BB    ", // 10
  "     BB   BB    ", // 11
  "     BB   BB    ", // 12
  "     BB   BB    ", // 13
  "                ", // 14
  "                ", // 14
  "                ", // 15
  "                ", // 16
};

static char *xpm_stop[] = {
  "16 16 2 1", // 16x16, 2 colors, 1 char/color
  "  c None",
  "B c #000000000000",
  "                ", // 1
  "                ", // 2
  "                ", // 3
  "                ", // 4
  "   BBBBBBBBBB   ", // 5
  "   BBBBBBBBBB   ", // 6
  "   BBBBBBBBBB   ", // 7
  "   BBBBBBBBBB   ", // 8
  "   BBBBBBBBBB   ", // 9
  "   BBBBBBBBBB   ", // 10
  "   BBBBBBBBBB   ", // 11
  "   BBBBBBBBBB   ", // 12
  "   BBBBBBBBBB   ", // 13
  "                ", // 14
  "                ", // 14
  "                ", // 15
  "                ", // 16
};

static char *xpm_speaker[] = {
  "16 16 2 1", // 16x16, 2 colors, 1 char/color
  "  c None",
  "B c #000000000000",
  "         B      ", // 1
  "        BB     B", // 2
  "       BBB    B ", // 3
  "      BBBB   B  ", // 4
  "     BBBBB  B   ", // 5
  "   BBBBBBB B    ", // 6
  "BBBBBBBBBB      ", // 7
  "BBBBBBBBBB      ", // 8
  "BBBBBBBBBB BBBBB", // 9
  "BBBBBBBBBB      ", // 10
  "   BBBBBBB      ", // 11
  "     BBBBB B    ", // 12
  "      BBBB  B   ", // 13
  "       BBB   B  ", // 14
  "        BB    B ", // 14
  "         B     B", // 15
  "                ", // 16
};

static char *xpm_speaker_muted[] = {
  "16 16 4 1", // 16x16, 2 colors, 1 char/color
  "  c None",
  "B c #000000000000",
  "r c #FFFF00000000",
  "R c #FFFF00000000",
  "         B     R", // 1
  "        BB    RR", // 2
  "       BBB   RR ", // 3
  "      BBBB  RR  ", // 4
  "     BBBBB RR   ", // 5
  "   BBBBBBBRR    ", // 6
  "BBBBBBBBBRR     ", // 7
  "BBBBBBBBRR      ", // 8
  "BBBBBBBRRB      ", // 9
  "BBBBBBRRBB      ", // 10
  "   BBRRBBB      ", // 11
  "    RRBBBB      ", // 12
  "   RR BBBB      ", // 13
  "  RR   BBB      ", // 14
  " RR     BB      ", // 14
  "RR       B      ", // 15
  "R               ", // 16
};





