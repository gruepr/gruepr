#ifndef CUSTOMSPLITTER_H
#define CUSTOMSPLITTER_H

#include "qsplitter.h"
#include "widgets/customsplitterhandle.h"
class CustomSplitter : public QSplitter {
public:
    CustomSplitter(Qt::Orientation orientation, QWidget *parent = nullptr);

protected:
    QSplitterHandle *createHandle() override;
};


#endif // CUSTOMSPLITTER_H
