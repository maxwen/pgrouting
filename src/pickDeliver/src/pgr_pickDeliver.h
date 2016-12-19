/*PGR-GNU*****************************************************************

FILE: pgr_pickDeliver.h

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

#ifndef SRC_PICKDELIVER_SRC_PGR_PICKDELIVER_H_
#define SRC_PICKDELIVER_SRC_PGR_PICKDELIVER_H_
#pragma once


#include "./../../common/src/pgr_types.h"

#include <string>
#include <vector>
#include <sstream>

#include "./vehicle_node.h"
#include "./order.h"
#include "./fleet.h"
#include "./solution.h"

namespace pgrouting {
namespace vrp {


class Optimize;
class Initial_solution;

class Pgr_pickDeliver {
    friend class Vehicle_pickDeliver;
    friend class Optimize;
    friend class Initial_solution;
    friend class Solution;
    friend class Fleet;
    typedef size_t ID;

 public:
    Pgr_pickDeliver(
            const std::vector<PickDeliveryOrders_t> &pd_orders,
            const std::vector<Vehicle_t> &vehicles,
            size_t max_cycles,
            std::string &error);

    void solve();

    std::vector<General_vehicle_orders_t>
        get_postgres_result() const;

    /*****************/

    const Order order_of(const Vehicle_node &node) const;
    const Vehicle_node& node(ID id) const;
    const std::vector<Order>& orders() const {return m_orders;}
    double speed() const {return m_speed;}

    /*! \brief get_log
     *
     * \returns the current contents of the log and clears the log
     *
     */
    std::ostream& get_log(std::ostream &p_log) const {
        p_log << log.str() << "\n";
        log.str("");
        log.clear();
        return p_log;
    }

    Solution solve(const Solution init_solution);
    size_t max_cycles() const {return m_max_cycles;}

 private:
    double max_capacity;
    double m_speed;
    size_t m_max_cycles;
    size_t max_vehicles;
    Vehicle_node m_starting_site, m_ending_site;
    std::vector<PickDeliveryOrders_t> m_original_data;
    std::vector<Vehicle_node> m_nodes;
    Fleet m_trucks;
    std::vector<Order> m_orders;
    std::vector<Solution> solutions;
 protected:
    mutable std::ostringstream log;
    mutable std::ostringstream notice;
    mutable std::ostringstream error;
#ifndef NDEBUG
    mutable std::ostringstream dbg_log;
#endif
};

}  //  namespace vrp
}  //  namespace pgrouting

#endif  // SRC_PICKDELIVER_SRC_PGR_PICKDELIVER_H_
