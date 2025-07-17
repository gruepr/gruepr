#ifndef CUSTOMSPLITTER_H
#define CUSTOMSPLITTER_H

#include <QSplitter>

class CustomSplitter : public QSplitter
{

public:
    CustomSplitter(Qt::Orientation orientation, QWidget *parent = nullptr);

protected:
    QSplitterHandle *createHandle() override;
};


#endif // CUSTOMSPLITTER_H
