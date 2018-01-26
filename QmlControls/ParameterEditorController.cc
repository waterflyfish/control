/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "ParameterEditorController.h"
#include "AutoPilotPluginManager.h"
#include "QGCApplication.h"
#include "ParameterManager.h"

#ifndef __mobile__
#include "QGCFileDialog.h"
#include "QGCMapRCToParamDialog.h"
#include "MainWindow.h"
#endif

#include <QStandardPaths>

/// @Brief Constructs a new ParameterEditorController Widget. This widget is used within the PX4VehicleConfig set of screens.
ParameterEditorController::ParameterEditorController(void)
    : _currentComponentId(_vehicle->defaultComponentId())
    , _parameters(new QmlObjectListModel(this))
{
    const QMap<int, QMap<QString, QStringList> >& groupMap = _vehicle->parameterManager()->getGroupMap();
    foreach (int componentId, groupMap.keys()) {
        _componentIds += QString("%1").arg(componentId);
    }

    _currentGroup = groupMap[_currentComponentId].keys()[0];
    _updateParameters();

    connect(this, &ParameterEditorController::searchTextChanged, this, &ParameterEditorController::_updateParameters);
    connect(this, &ParameterEditorController::currentComponentIdChanged, this, &ParameterEditorController::_updateParameters);
    connect(this, &ParameterEditorController::currentGroupChanged, this, &ParameterEditorController::_updateParameters);
}

ParameterEditorController::~ParameterEditorController()
{
    
}

QStringList ParameterEditorController::getGroupsForComponent(int componentId)
{
    const QMap<int, QMap<QString, QStringList> >& groupMap = _vehicle->parameterManager()->getGroupMap();

    return groupMap[componentId].keys();
}

QStringList ParameterEditorController::getParametersForGroup(int componentId, QString group)
{
    const QMap<int, QMap<QString, QStringList> >& groupMap = _vehicle->parameterManager()->getGroupMap();

    return groupMap[componentId][group];
}

QStringList ParameterEditorController::searchParametersForComponent(int componentId, const QString& searchText, bool searchInName, bool searchInDescriptions)
{
    QStringList list;
    
    foreach(const QString &paramName, _vehicle->parameterManager()->parameterNames(componentId)) {
        if (searchText.isEmpty()) {
            list += paramName;
        } else {
            Fact* fact = _vehicle->parameterManager()->getParameter(componentId, paramName);
            
            if (searchInName && fact->name().contains(searchText, Qt::CaseInsensitive)) {
                list += paramName;
            } else if (searchInDescriptions && (fact->shortDescription().contains(searchText, Qt::CaseInsensitive) || fact->longDescription().contains(searchText, Qt::CaseInsensitive))) {
                list += paramName;
            }
        }
    }
    list.sort();
    
    return list;
}

void ParameterEditorController::clearRCToParam(void)
{
    Q_ASSERT(_uas);
    _uas->unsetRCToParameterMap();
}

void ParameterEditorController::saveToFile(const QString& filename)
{
    if (!_autopilot) {
        qWarning() << "Internal error _autopilot==NULL";
        return;
    }

    if (!filename.isEmpty()) {
        QFile file(filename);
        
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qgcApp()->showMessage(QString("Unable to create file: %1").arg(filename));
            return;
        }
        
        QTextStream stream(&file);
        _vehicle->parameterManager()->writeParametersToStream(stream);
        file.close();
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
           qDebug()<<"the file can not open !";
        }
        QByteArray text = file.readAll().toBase64();
        file.close();
        file.remove();
        if(text.isEmpty()){
            qDebug()<<"the file read failed";
        }
        QFile file1(filename);
        if(!file1.open(QIODevice::WriteOnly | QIODevice::Text)){
            qDebug()<<"file create failed!";
        }
        file1.write(text);
        file1.close();
//        QTextStream stream1(&file);

//        while(!stream1.atEnd()){
//            QString line = stream1.readLine();
//            qDebug()<<line;
//            QByteArray source;
//            source.append(line);
//            qDebug()<<source<<"+++2";
//            QByteArray encrypt =  source.toBase64();
//            qDebug()<<encrypt;
////            file.write(encrypt);
    }
}

void ParameterEditorController::saveToFilePicker(void)
{
#ifndef __mobile__
    QString fileName = QGCFileDialog::getSaveFileName(NULL,
                                                      "Save Parameters",
                                                      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                      "Parameter Files (*.params)",
                                                      "params",
                                                      true);
    saveToFile(fileName);
#endif
}

void ParameterEditorController::loadFromFile(const QString& filename)
{
    QString errors;
    
    if (!_autopilot) {
        qWarning() << "Internal error _autopilot==NULL";
        return;
    }

    if (!filename.isEmpty()) {
        QFile file(filename);
        
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qgcApp()->showMessage(QString("Unable to open file: %1").arg(filename));
            return;
        }
        QByteArray text;
        text = text.fromBase64(file.readAll());
        QTextStream stream(text);
        errors = _vehicle->parameterManager()->readParametersFromStream(stream);
        file.close();
        
        if (!errors.isEmpty()) {
            emit showErrorMessage(errors);
        }else{
            qgcApp()->showMessage(tr("Parameters load successfully!"));
        }
    }
}

void ParameterEditorController::loadFromFilePicker(void)
{
#ifndef __mobile__
    QString fileName = QGCFileDialog::getOpenFileName(NULL,
                                                      "Load Parameters",
                                                      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                      "Parameter Files (*.params);;All Files (*)");
    loadFromFile(fileName);
#endif
}

void ParameterEditorController::refresh(void)
{
    _vehicle->parameterManager()->refreshAllParameters();
}

void ParameterEditorController::resetAllToDefaults(void)
{
    _vehicle->parameterManager()->resetAllParametersToDefaults();
    refresh();
}

void ParameterEditorController::setRCToParam(const QString& paramName)
{
#ifdef __mobile__
    Q_UNUSED(paramName)
#else
    Q_ASSERT(_uas);
    QGCMapRCToParamDialog * d = new QGCMapRCToParamDialog(paramName, _uas, qgcApp()->toolbox()->multiVehicleManager(), MainWindow::instance());
    d->exec();
#endif
}

void ParameterEditorController::_updateParameters(void)
{
    QObjectList newParameterList;
    //int i=0;
    if (_searchText.isEmpty()) {
        const QMap<int, QMap<QString, QStringList> >& groupMap = _vehicle->parameterManager()->getGroupMap();
        foreach (const QString& parameter, groupMap[_currentComponentId][_currentGroup]) {
           // if(_vehicle->parameterManager()->getParameter(_vehicle->defaultComponentId(), parameter)->name()=="MNT_PITCH_P")qDebug()<<"NAE=="<<i;
           // i++;
            newParameterList.append(_vehicle->parameterManager()->getParameter(_currentComponentId, parameter));
        }
    } else {
        foreach(const QString &parameter, _vehicle->parameterManager()->parameterNames(_vehicle->defaultComponentId())) {
            Fact* fact = _vehicle->parameterManager()->getParameter(_vehicle->defaultComponentId(), parameter);
            if (fact->name().contains(_searchText, Qt::CaseInsensitive) ||
                    fact->shortDescription().contains(_searchText, Qt::CaseInsensitive) ||
                    fact->longDescription().contains(_searchText, Qt::CaseInsensitive)) {

                newParameterList.append(fact);
            }
        }
    }
   Fact* f ;

    _parameters->swapObjectList(newParameterList);

    //dynamic_cast<Fact*>(_parameters->get(35))->setCookedValue(5);
}
void ParameterEditorController::setPitch_p(float pitch_p){
    //qDebug()<<"KKKKKKKKKKKKKKKkk==";
      _pitch_p = pitch_p;
      //_updateParameters();
     // qDebug()<<"name==="<<dynamic_cast<Fact*>(_parameters->get(35))->name();
      emit dynamic_cast<Fact*>(_parameters->get(35))->valueChanged(pitch_p);//->setCookedValue(pitch_p);
      emit pitch_pChanged();
}
