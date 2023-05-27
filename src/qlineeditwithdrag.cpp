#include "qlineeditwithdrag.h"

QLineEditWithDrag::QLineEditWithDrag(QWidget *parent)
    : QLineEdit (parent)
{

}

void QLineEditWithDrag::dragEnterEvent(QDragEnterEvent *e)
{
    if (true)
    {
        e->acceptProposedAction();
    }
}

void QLineEditWithDrag::dropEvent(QDropEvent *e)
{
    QList<QUrl> urls = e->mimeData()->urls();
    if (urls.isEmpty())
        return;

    QString url = urls.first().toLocalFile();
    setText(url);
}
