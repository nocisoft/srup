#ifndef NSFILESELECT_H
#define NSFILESELECT_H

#include <QWidget>

class QToolButton;
class QHBoxLayout;
class QLineEdit;

class NSFileSelect : public QWidget
{
    Q_OBJECT
public:
    NSFileSelect(QWidget* parent = 0 );
    ~NSFileSelect();

    enum SelectModes { OpenFile=0, SaveFile, OpenDir };

    void setSelectMode( const SelectModes m ) { m_mode = m; }
    SelectModes selectMode() const { return m_mode; }
    void setFilter( const QString& );
    void setFileName( const QString& );
    QString fileName() const;
    QLineEdit* fileNameEditor();

protected slots:
    /*$PROTECTED_SLOTS$*/
    void selectFile();

private:
    QToolButton *btnSelect;
    QHBoxLayout *layout;
    QLineEdit *leFileName;
    QString m_filter;
    SelectModes m_mode;
};

#endif // NSFILESELECT_H
