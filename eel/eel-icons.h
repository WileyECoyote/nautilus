
#ifndef EEL_ICONS_EXTENSIONS_H
#define EEL_ICONS_EXTENSIONS_H

/*!
 * \file eel-icons.h: Consolidated gsetting API with fail-safe.
 *
 * Copyright (C) 2014 Wiley Edward Hill <wileyhill@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * \remarks This files is included by sources only requiring the
 * icon size enumeration and not all of GTK's headers.
 *
 */

/* Built-in stock icon sizes */
typedef enum
{
  STD_ICON_SIZE_INVALID,
  STD_ICON_SIZE_MENU,
  STD_ICON_SIZE_SMALL_TOOLBAR,
  STD_ICON_SIZE_LARGE_TOOLBAR,
  STD_ICON_SIZE_BUTTON,
  STD_ICON_SIZE_DND,
  STD_ICON_SIZE_DIALOG
} StdIconSize;

#endif
