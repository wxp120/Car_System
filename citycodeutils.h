#ifndef CITYCODEUTILS_H
#define CITYCODEUTILS_H

#include <QMap>
#include <QString>

class CitycodeUtils
{
public:
    CitycodeUtils();
    QMap<QString,QString> citymap;
    QString getcityCodeFromName(QString name);
    void initCityMap();
};

#endif // CITYCODEUTILS_H
