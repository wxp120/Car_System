#include "citycodeutils.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

CitycodeUtils::CitycodeUtils()
{

}

QString CitycodeUtils::getcityCodeFromName(QString name)
{
    if(citymap.isEmpty())
    {
        initCityMap();
    }
    auto found = citymap.find(name);
    if(found == citymap.end())
    {
        found = citymap.find(name+"市");
        if(found == citymap.end())
        {
            found = citymap.find(name+"县");
            if(found == citymap.end())
            {
                found = citymap.find(name+"区");
                if(found == citymap.end())
                {
                     return "";
                }
            }
        }
    }
    return found.value();
}

void CitycodeUtils::initCityMap()
{
    QFile file(":/citycode.json");
    file.open(QIODevice::ReadOnly);
    QByteArray rawData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(rawData);
    if(jsonDoc.isArray()) {
        QJsonArray city = jsonDoc.array();
        for(QJsonValue value:city){
            if(value.isObject())
            {
                QString cityName = value["city_name"].toString();
                QString cityCode = value["city_code"].toString();
                citymap[cityName] = cityCode;
            }
        }

    }
}
