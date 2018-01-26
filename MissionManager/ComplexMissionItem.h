/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef ComplexMissionItem_H
#define ComplexMissionItem_H

#include "VisualMissionItem.h"

class ComplexMissionItem : public VisualMissionItem
{
    Q_OBJECT

public:
    ComplexMissionItem(Vehicle* vehicle, QObject* parent = NULL);

    const ComplexMissionItem& operator=(const ComplexMissionItem& other);

    Q_PROPERTY(int      lastSequenceNumber  READ lastSequenceNumber NOTIFY lastSequenceNumberChanged)
    Q_PROPERTY(double   complexDistance     READ complexDistance    NOTIFY complexDistanceChanged)

    /// @return The distance covered the complex mission item in meters.
    virtual double complexDistance(void) const = 0;

    /// @return The last sequence number used by this item. Takes into account child items of the complex item
    virtual int lastSequenceNumber(void) const = 0;

    /// Update the mission items associated with the complex item.
    /// must be called before call function getMissionItems
    virtual void updateMissionItems(bool update = false) = 0;

    /// Returns the mission items associated with the complex item. Caller is responsible for freeing. Calling
    /// delete on returned QmlObjectListModel will free all memory including internal items.
    virtual QmlObjectListModel* getMissionItems(QGeoCoordinate itemCoordiante) const = 0;
    virtual QmlObjectListModel* getVtolMissionItems(void) const = 0;

    /// Load the complex mission item from Json
    ///     @param complexObject Complex mission item json object
    ///     @param[out] errorString Error if load fails
    /// @return true: load success, false: load failed, errorString set
    virtual bool load(const QJsonObject& complexObject, QString& errorString) = 0;

    /* Changed by chu.fumin 2016122711 start : dms degree,minute,second convert */
    /// Load the complex mission item from Coordinate of VariantList,
    ///     @param polygonPath Coordinate item
    ///     @param seq Coordinate seq
    /// @return true: load success, false: load failed
    /// Item is dms, example :
    /// - <i>LEGAL</i> (these are all equivalent)
    /// - 070:00:45, 70:01:15W+0:0.5, 70:01:15W-0:0:30W, W70:01:15+0:0:30E
    /// - <i>ILLEGAL</i> (the exception thrown explains the problem)
    /// - 70:01:15W+0:0:15N, W70:01:15+W0:0:15
    virtual bool loadDMS(const QVariantList &polygonPath, unsigned int seq) = 0;
    /* Changed by chu.fumin 2016122711 end : dms degree,minute,second convert */

    /// Get the point of complex mission item furthest away from a coordinate
    ///     @param other QGeoCoordinate to which distance is calculated
    /// @return the greatest distance from any point of the complex item to some coordinate
    virtual double greatestDistanceTo(const QGeoCoordinate &other) const = 0;

    virtual QVariantList&   getCameraTriggerPointList() = 0;

signals:
    void lastSequenceNumberChanged  (int lastSequenceNumber);
    void complexDistanceChanged     (double complexDistance);
};

#endif
