/****************************************************************************
* 
*  Copyright (C) 2002-2008 Helta Kft. / NociSoft Software Solutions
*  All rights reserved.
*  Author: Norbert Szabo
*  E-mail: nszabo@helta.hu, info@nocisoft.com
*  Web: www.nocisoft.com
* 
*  This file is part of the NCReport reporting software
* 
*  Licensees holding a valid NCReport License Agreement may use this
*  file in accordance with the rights, responsibilities, and obligations
*  contained therein. Please consult your licensing agreement or contact
*  nszabo@helta.hu if any conditions of this licensing are not clear
*  to you.
* 
*  This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
*  WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* 
****************************************************************************/
#include "ncrdsqltextedit.h"
#include "ncrdsqleditorhighlighter.h"

#include <QFocusEvent>

NCRDSqlTextEdit::NCRDSqlTextEdit(QWidget * parent) : QTextEdit( parent )
{
	setFont( QFont("Courier New",10) );
}


NCRDSqlTextEdit::~NCRDSqlTextEdit()
{
}

void NCRDSqlTextEdit::focusOutEvent(QFocusEvent * event)
{
	QTextEdit::focusOutEvent( event );
	if ( event->reason() == Qt::MouseFocusReason ) {
	}
	emit outFocus();
}

void NCRDSqlTextEdit::setAdjustedText(const QString & text )
{
	QString txt( text );
	txt.replace('\t', "" );
	setText( txt );
}

void NCRDSqlTextEdit::installHighlighter()
{
	new NCRDSqlEditorHighlighter(document());
}

