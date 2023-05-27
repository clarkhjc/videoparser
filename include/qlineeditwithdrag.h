#ifndef QLINEEDITWITHDRAG_H
#define QLINEEDITWITHDRAG_H

#include <QObject>
#include <QLineEdit>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

class QLineEditWithDrag : public QLineEdit
{
public:
    QLineEditWithDrag(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
};

#endif // QLINEEDITWITHDRAG_H
