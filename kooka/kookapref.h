#ifndef KOOKAPREF_H
#define KOOKAPREF_H

#include <kdialogbase.h>
#include <qframe.h>

class KookaPrefPageOne;
class KookaPrefPageTwo;

class KookaPreferences : public KDialogBase
{
    Q_OBJECT
public:
    KookaPreferences();

private:
    KookaPrefPageOne *m_pageOne;
    KookaPrefPageTwo *m_pageTwo;
};

class KookaPrefPageOne : public QFrame
{
    Q_OBJECT
public:
    KookaPrefPageOne(QWidget *parent = 0);
};

class KookaPrefPageTwo : public QFrame
{
    Q_OBJECT
public:
    KookaPrefPageTwo(QWidget *parent = 0);
};

#endif // KOOKAPREF_H
