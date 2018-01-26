#ifndef QGCUASFILEVIEWMULTI_H
#define QGCUASFILEVIEWMULTI_H

#include <QMap>

#include "QGCDockWidget.h"
#include "QGCUASFileView.h"
#include "UAS.h"
#include "QGCApplication.h"
namespace Ui
{
class QGCUASFileViewMulti;
}

class QGCUASFileViewMulti : public QGCDockWidget
{
    Q_OBJECT

public:
    explicit QGCUASFileViewMulti(const QString& title, QAction* action, QWidget *parent = 0);
    ~QGCUASFileViewMulti();

    void closeEvent(QCloseEvent* event){
        Vehicle *vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
        if(vehicle && vehicle->uas ()){
            QGCUASFileView *fileView = lists.value (vehicle->uas ());
            if(fileView){
                fileView->closeFileViewEvent ();
            }
        }
        _action->trigger();
        QWidget::closeEvent(event);
    }

protected:
    void changeEvent(QEvent *e);
    QMap<UAS*, QGCUASFileView*> lists;
    
private slots:
    void _vehicleAdded(Vehicle* vehicle);
    void _vehicleRemoved(Vehicle* vehicle);
    void _activeVehicleChanged(Vehicle* vehicle);

private:
    Ui::QGCUASFileViewMulti *ui;
};

#endif // QGCUASFILEVIEWMULTI_H
