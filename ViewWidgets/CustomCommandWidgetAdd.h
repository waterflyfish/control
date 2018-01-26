/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef CustomCommandWidgetAdd_H
#define CustomCommandWidgetAdd_H

#include "QGCQmlWidgetHolder.h"

class CustomCommandWidgetAdd : public QGCQmlWidgetHolder
{
    Q_OBJECT
	
public:
    CustomCommandWidgetAdd(const QString& title, QAction* action, QWidget *parent = 0);
};

#endif
