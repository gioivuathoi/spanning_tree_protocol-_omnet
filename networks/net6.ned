package networks;


//simple Txc2
//{
//    parameters:
//        @display("i=block/switch");
//    gates:
//        inout ports[];  // declare in[] and out[] to be vector gates
//}

//
// Using local channel type definition to reduce the redundancy
// of connection definitions.
//
network Net6
{
    @display("bgb=505,380");
    types:
        channel Channel extends ned.DelayChannel
        {
            delay = 500ms;
        }
    submodules:
        tic[6]: Txc1;

    connections:
        tic[5].ports++ <--> Channel <--> tic[1].ports++;
        tic[5].ports++ <--> Channel <--> tic[0].ports++;
        tic[1].ports++ <--> Channel <--> tic[2].ports++;
        tic[1].ports++ <--> Channel <--> tic[3].ports++;
        tic[1].ports++ <--> Channel <--> tic[4].ports++;
        tic[3].ports++ <--> Channel <--> tic[4].ports++;
        tic[4].ports++ <--> Channel <--> tic[0].ports++;

}
