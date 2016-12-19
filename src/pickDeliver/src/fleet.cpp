/*PGR-GNU*****************************************************************

FILE: solution.cpp

Copyright (c) 2015 pgRouting developers
Mail: project@pgrouting.org

------

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 ********************************************************************PGR-GNU*/


#include <string>
#include <vector>

#include "./fleet.h"
#include "./tw_node.h"
#include "./vehicle_pickDeliver.h"
#include "./pgr_pickDeliver.h"
#include "./../../common/src/identifiers.hpp"


namespace pgrouting {
namespace vrp {

void
Fleet::build_fleet(
        const std::vector<Vehicle_t> &vehicles,
        size_t &node_id
        ) {
    for (auto vehicle : vehicles) {

        if (vehicle.cant_v < 0) {
            problem->error << "Illegal number of vehicles found vehicle";
            problem->log << vehicle.cant_v << "< 0 on vehicle " << vehicle.id;
            continue;
        }

        auto starting_site = Vehicle_node(
                {node_id++, vehicle, Tw_node::NodeType::kStart, problem});
        auto ending_site = Vehicle_node(
                {node_id++, vehicle, Tw_node::NodeType::kEnd, problem});

        (problem->m_nodes).push_back(starting_site);
        (problem->m_nodes).push_back(ending_site);

        for (int i = 0; i < vehicle.cant_v; ++i) {
            m_trucks.push_back(Vehicle_pickDeliver(
                        vehicle.id,
                        starting_site,
                        ending_site,
                        vehicle.capacity,
                        vehicle.speed,
                        problem));
        }
    }
    Identifiers<size_t> unused(m_trucks.size());
    un_used = unused;
}

bool
Fleet::is_fleet_ok() const {
    for (auto truck : m_trucks) {
        if (!(truck.start_site().is_start()
                    && truck.end_site().is_end())) {
            problem->error << "Illegal values found on vehcile";
            return false;
        }
        if (!truck.is_feasable()) {
            problem->error << "Truck is not feasable";
            return false;
        }
    }
    return true;
}

bool
Fleet::is_order_ok(const Order &order) const {
    for (const auto truck : m_trucks) {
        auto test_truck = truck;
        test_truck.push_back(order);

        if (test_truck.is_feasable()) {
            return true;
        }
    }
    return false;
}

Vehicle_pickDeliver&
Fleet::operator[](size_t i) {
    pgassert(i < m_trucks.size());
    return m_trucks[i];
}
#if 0
std::vector<General_vehicle_orders_t>
Solution::get_postgres_result() const {
    std::vector<General_vehicle_orders_t> result;
    /* postgres numbering starts with 1 */
    int i(1);
    for (const auto truck : fleet) {
        std::vector<General_vehicle_orders_t> data =
            truck.get_postgres_result(i);
        result.insert(result.end(), data.begin(), data.end());

        ++i;
    }
    return result;
}



bool
Solution::is_feasable() const {
    for (const auto v : fleet) {
        if (v.is_feasable()) continue;
        return false;
    }
    return true;
}

double
Solution::duration() const {
    double total(0);
    for (const auto v : fleet) {
        total += v.duration();
    }
    return total;
}

int
Solution::twvTot() const {
    int total(0);
    for (const auto v : fleet) {
        total += v.twvTot();
    }
    return total;
}

double
Solution::wait_time() const {
    double total(0);
    for (const auto v : fleet) {
        total += v.total_wait_time();
    }
    return total;
}

double
Solution::total_travel_time() const {
    double total(0);
    for (const auto v : fleet) {
        total += v.total_travel_time();
    }
    return total;
}

double
Solution::total_service_time() const {
    double total(0);
    for (const auto v : fleet) {
        total += v.total_service_time();
    }
    return total;
}

int
Solution::cvTot() const {
    int total(0);
    for (const auto v : fleet) {
        total += v.cvTot();
    }
    return total;
}

Vehicle::Cost
Solution::cost() const {
    double total_duration(0);
    double total_wait_time(0);
    int total_twv(0);
    int total_cv(0);
    for (const auto v : fleet) {
        total_duration += v.duration();
        total_wait_time += v.total_wait_time();
        total_twv += v.twvTot();
        total_cv += v.cvTot();
    }
    return std::make_tuple(
            total_twv, total_cv, fleet.size(),
            total_wait_time, total_duration);
}



std::string
Solution::cost_str() const {
    Vehicle::Cost s_cost(cost());
    std::ostringstream log;

    log << "(twv, cv, fleet, wait, duration) = ("
        << std::get<0>(s_cost) << ", "
        << std::get<1>(s_cost) << ", "
        << std::get<2>(s_cost) << ", "
        << std::get<3>(s_cost) << ", "
        << std::get<4>(s_cost) << ")";

    return log.str();
}

std::string
Solution::tau(const std::string &title) const {
    Vehicle::Cost s_cost(cost());
    std::ostringstream log;

    log << "\n" << title << ": " << std::endl;
    for (const auto v : fleet) {
        log << "\n" << v.tau();
    }
    log << "\n" << cost_str() << "\n";
    return log.str();
}

std::ostream&
operator << (std::ostream &log, const Solution &solution) {
    for (const auto vehicle : solution.fleet) {
        log << vehicle;
    }

    log << "\n SOLUTION:\n\n "
        << solution.tau();

    return log;
}

bool
Solution::operator<(const Solution &s_rhs) const {
    Vehicle::Cost lhs(cost());
    Vehicle::Cost rhs(s_rhs.cost());

    /*
     * capacity violations
     */
    if (std::get<0>(lhs) < std::get<0>(rhs))
        return true;
    if (std::get<0>(lhs) > std::get<0>(rhs))
        return false;

    /*
     * time window violations
     */
    if (std::get<1>(lhs) < std::get<1>(rhs))
        return true;
    if (std::get<1>(lhs) > std::get<1>(rhs))
        return false;

    /*
     * fleet size
     */
    if (std::get<2>(lhs) < std::get<2>(rhs))
        return true;
    if (std::get<2>(lhs) > std::get<2>(rhs))
        return false;

    /*
     * waiting time
     */
    if (std::get<3>(lhs) < std::get<3>(rhs))
        return true;
    if (std::get<3>(lhs) > std::get<3>(rhs))
        return false;

    /*
     * duration
     */
    if (std::get<4>(lhs) < std::get<4>(rhs))
        return true;
    if (std::get<4>(lhs) > std::get<4>(rhs))
        return false;

    return false;
}
#endif
}  //  namespace vrp
}  //  namespace pgrouting
