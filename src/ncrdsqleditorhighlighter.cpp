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
#include "ncrdsqleditorhighlighter.h"

#include <QtGui>

static const char * const _keywords[] = 
{
"ADD","ALTER","AND","AS", 
"BEGIN","BY",
"CASCADE","CLOSE","COMMIT","CREATE","CURSOR",
"DECLARE","DEFAULT","DELETE","DESC","DISTINCT","DROP",
"EXECUTE","EXISTS","EXTRACT",
"FETCH","FLOAT", "FOR", "FROM", "FULL",
"GRANT",
"HAVING",
"IN","INNER","INSERT","INTERVAL","INTO","IS",
"JOIN", 
"LEADING","LEFT","LIKE","LOCAL",
"NAMES","NATIONAL","NATURAL","NCHAR","NO","NOT","NULL",
"ON","OR","OUTER",
"PARTIAL","PRIMARY","PRIVILEGES","PROCEDURE","PUBLIC",
"REFERENCES","REVOKE","RIGHT","ROLLBACK",
"SELECT","SET","SUBSTRING",
"TO","TRAILING","TRIM"
"UNION","UNIQUE","UPDATE","USING",
"VALUES","VARCHAR","VARYING","VIEW",
"WHERE","WITH","WORK",
"GROUP", "ORDER",
"$P","$V","$D",
0};


NCRDSqlEditorHighlighter::NCRDSqlEditorHighlighter(QTextDocument *parent)
	: QSyntaxHighlighter(parent)
{
	HighlightingRule rule;

	keywordFormat.setForeground(Qt::darkBlue);
	keywordFormat.setFontWeight(QFont::Bold);
	QStringList keywordPatterns;
	int i=0;
	while ( _keywords[i] ) {
		QString s(_keywords[i++]);
		QString q;
		for ( int i=0; i< s.length(); ++i )
			q += "["+s.mid(i,1).toUpper()+s.mid(i,1).toLower()+"]";
		
		keywordPatterns << QString("\\b")+q+"\\b";
	}
	foreach (QString pattern, keywordPatterns) {
		rule.pattern = QRegExp(pattern);
		rule.format = keywordFormat;
		highlightingRules.append(rule);
	}

	classFormat.setFontWeight(QFont::Bold);
	classFormat.setForeground(Qt::darkMagenta);
	rule.pattern = QRegExp("\\bQ[A-Za-z]+\\b");
	rule.format = classFormat;
	highlightingRules.append(rule);

	singleLineCommentFormat.setForeground(Qt::darkGray);
	rule.pattern = QRegExp("//[^\n]*");
	rule.format = singleLineCommentFormat;
	highlightingRules.append(rule);

	multiLineCommentFormat.setForeground(Qt::darkGray);

	quotationFormat.setForeground(Qt::red);
	rule.pattern = QRegExp("[\"\'].*[\"\']");
	rule.format = quotationFormat;
	highlightingRules.append(rule);

	functionFormat.setFontItalic(true);
	functionFormat.setFontWeight(QFont::Bold);
	functionFormat.setForeground(Qt::blue);
	rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
	rule.format = functionFormat;
	highlightingRules.append(rule);

	numberFormat.setForeground(Qt::darkGreen);
	rule.pattern = QRegExp("\\b[0-9.]+\\b");
	rule.format = numberFormat;
	highlightingRules.append(rule);
	
	commentStartExpression = QRegExp("/\\*");
	commentEndExpression = QRegExp("\\*/");
}

void NCRDSqlEditorHighlighter::highlightBlock(const QString &text)
{
	foreach (HighlightingRule rule, highlightingRules) {
		QRegExp expression(rule.pattern);
		int index = text.indexOf(expression);
		while (index >= 0) {
			int length = expression.matchedLength();
			setFormat(index, length, rule.format);
			index = text.indexOf(expression, index + length);
		}
	}
	setCurrentBlockState(0);

	int startIndex = 0;
	if (previousBlockState() != 1)
		startIndex = text.indexOf(commentStartExpression);

	while (startIndex >= 0) {
		int endIndex = text.indexOf(commentEndExpression, startIndex);
		int commentLength;
		if (endIndex == -1) {
			setCurrentBlockState(1);
			commentLength = text.length() - startIndex;
		} else {
			commentLength = endIndex - startIndex
							+ commentEndExpression.matchedLength();
		}
		setFormat(startIndex, commentLength, multiLineCommentFormat);
		startIndex = text.indexOf(commentStartExpression,
												startIndex + commentLength);
	}
}
