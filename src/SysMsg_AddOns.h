/***************************************************************************
                              SysMsg_AddOns.h
                     Deklarationen von Datenstrukturen
                             -------------------
    begin                : Mon Jul 25 2005
    copyright            : (C) 2005 by Martin Renwanz
    email                : renwanz@tkn.tu-berlin.de
 ***************************************************************************/
 
// build structure for SysMsg container
struct Container{
  cQueue* list;
  int** allocTable;
};

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

