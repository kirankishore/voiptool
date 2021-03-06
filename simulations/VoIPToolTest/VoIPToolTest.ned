//
// Copyright (C) 2006 Andras Varga
//
// This library is free software, you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation;
// either version 2 of the License, or any later version.
// The library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//

package VoIPToolTest;

import inet.nodes.inet.StandardHost;
import inet.nodes.inet.Router;
import inet.nodes.ethernet.EtherSwitch;
import inet.networklayer.autorouting.FlatNetworkConfigurator;


channel line1 extends ned.DatarateChannel
{
    parameters:
        delay = 100us;
//        datarate = 10Mbps;
}

channel line2 extends ned.DatarateChannel
{
    parameters:
        delay = 100us;
//        datarate = 10Mbps;
}


network VoIPToolTest
{
    submodules:
        client: StandardHost {
            parameters:
                @display("p=71,64;i=device/laptop_l");
        }
        switch: EtherSwitch {
            parameters:
                @display("p=202,156;i=device/switch");
        }
        net: Router {
            parameters:
                @display("p=394,166;i=misc/cloud_l");
        }
        router: Router {
            parameters:
                @display("p=311,74;i=abstract/router");
        }
        server: StandardHost {
            parameters:
                @display("p=512,58;i=device/server_l");
        }
        configurator: FlatNetworkConfigurator {
            parameters:
                networkAddress = "10.0.0.0";
                netmask = "255.255.0.0";
                @display("p=495,160;i=block/cogwheel_s");
        }
    connections:
        client.ethg++ <--> line1 <--> switch.ethg++;
        router.ethg++ <--> line1 <--> switch.ethg++;
        router.ethg++ <--> line2 <--> net.ethg++;
        server.ethg++ <--> line2 <--> net.ethg++;
}
