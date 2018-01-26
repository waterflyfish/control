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

#include "AutoPilotPlugin.h"
#include "QGCApplication.h"
#include "ParameterManager.h"
#include "UAS.h"
#include "FirmwarePlugin.h"

AutoPilotPlugin::AutoPilotPlugin(Vehicle* vehicle, QObject* parent)
    : QObject(parent)
    , _vehicle(vehicle)
    , _firmwarePlugin(vehicle->firmwarePlugin())
	, _setupComplete(false)
{

}

AutoPilotPlugin::~AutoPilotPlugin()
{
    
}

void AutoPilotPlugin::_recalcSetupComplete(void)
{
	bool newSetupComplete = true;
	
	foreach(const QVariant componentVariant, vehicleComponents()) {
		VehicleComponent* component = qobject_cast<VehicleComponent*>(qvariant_cast<QObject *>(componentVariant));
		Q_ASSERT(component);
		
		if (!component->setupComplete()) {
			newSetupComplete = false;
			break;
		}
	}
	
	if (_setupComplete != newSetupComplete) {
		_setupComplete = newSetupComplete;
		emit setupCompleteChanged(_setupComplete);
	}
}

bool AutoPilotPlugin::setupComplete(void)
{
	return _setupComplete;
}

void AutoPilotPlugin::parametersReadyPreChecks(void)
{
    _recalcSetupComplete();
    if (!_setupComplete) {
        qgcApp()->showMessage(tr("One or more vehicle components require setup prior to flight."));

        // Take the user to Vehicle Summary
        qgcApp()->showSetupView();
        qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
}
