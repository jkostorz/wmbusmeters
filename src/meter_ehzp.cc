/*
 Copyright (C) 2020 Fredrik Öhrström

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include"dvparser.h"
#include"meters.h"
#include"meters_common_implementation.h"
#include"wmbus.h"
#include"wmbus_utils.h"
#include"util.h"

struct MeterEHZP : public virtual ElectricityMeter, public virtual MeterCommonImplementation {
    MeterEHZP(WMBus *bus, MeterInfo &mi);

    double totalEnergyConsumption(Unit u);
    double currentPowerConsumption(Unit u);
    double totalEnergyProduction(Unit u);
    double currentPowerProduction(Unit u);

private:

    void processContent(Telegram *t);

    double total_energy_kwh_ {};
    double current_power_kw_ {};
    double total_energy_returned_kwh_ {};
    double current_power_returned_kw_ {};
    double on_time_h_ {};
};

MeterEHZP::MeterEHZP(WMBus *bus, MeterInfo &mi) :
    MeterCommonImplementation(bus, mi, MeterType::EHZP, MANUFACTURER_EMH)
{
    setExpectedTPLSecurityMode(TPLSecurityMode::AES_CBC_IV);

    // This is one manufacturer of EHZP compatible meters.
    addManufacturer(MANUFACTURER_APA);
    addMedia(0x02); // Electricity meter

    // This is another manufacturer
    addManufacturer(MANUFACTURER_DEV);
    // Oddly, this device has not been configured to send as a electricity meter,
    // but instead a device/media type that is used for gateway or relays or something?
    addMedia(0x37); // Radio converter (meter side)

    addLinkMode(LinkMode::T1);

    setExpectedVersion(0x02);

    addPrint("total_energy_consumption", Quantity::Energy,
             [&](Unit u){ return totalEnergyConsumption(u); },
             "The total energy consumption recorded by this meter.",
             true, true);

    addPrint("current_power_consumption", Quantity::Power,
             [&](Unit u){ return currentPowerConsumption(u); },
             "Current power consumption.",
             true, true);

    addPrint("total_energy_production", Quantity::Energy,
             [&](Unit u){ return totalEnergyProduction(u); },
             "The total energy production recorded by this meter.",
             true, true);

    addPrint("on_time", Quantity::Time,
             [&](Unit u){ assertQuantity(u, Quantity::Time);
                 return convert(on_time_h_, Unit::Hour, u); },
             "Device on time.",
             true, true);
}

unique_ptr<ElectricityMeter> createEHZP(WMBus *bus, MeterInfo &mi)
{
    return unique_ptr<ElectricityMeter>(new MeterEHZP(bus, mi));
}

double MeterEHZP::totalEnergyConsumption(Unit u)
{
    assertQuantity(u, Quantity::Energy);
    return convert(total_energy_kwh_, Unit::KWH, u);
}

double MeterEHZP::currentPowerConsumption(Unit u)
{
    assertQuantity(u, Quantity::Power);
    return convert(current_power_kw_, Unit::KW, u);
}

double MeterEHZP::totalEnergyProduction(Unit u)
{
    assertQuantity(u, Quantity::Energy);
    return convert(total_energy_returned_kwh_, Unit::KWH, u);
}

double MeterEHZP::currentPowerProduction(Unit u)
{
    assertQuantity(u, Quantity::Power);
    return convert(current_power_returned_kw_, Unit::KW, u);
}

void MeterEHZP::processContent(Telegram *t)
{
    int offset;
    string key;

    if (findKey(MeasurementType::Unknown, ValueInformation::EnergyWh, 0, &key, &t->values))
    {
        extractDVdouble(&t->values, key, &offset, &total_energy_kwh_);
        t->addMoreExplanation(offset, " total energy (%f kwh)", total_energy_kwh_);
    }

    if (findKey(MeasurementType::Unknown, ValueInformation::PowerW, 0, &key, &t->values))
    {
        extractDVdouble(&t->values, key, &offset, &current_power_kw_);
        t->addMoreExplanation(offset, " current power (%f kw)", current_power_kw_);
    }

    extractDVdouble(&t->values, "07803C", &offset, &total_energy_returned_kwh_);
    t->addMoreExplanation(offset, " total energy returned (%f kwh)", total_energy_returned_kwh_);

    extractDVdouble(&t->values, "0420", &offset, &on_time_h_);
    t->addMoreExplanation(offset, " on time (%f h)", on_time_h_);

}
