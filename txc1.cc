#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "spaningtree_m.h"

using namespace omnetpp;


void copy_id(char *dest, std::string source)
{
    for(int i = 0; i<4; i++)
    {
        dest[i] = source[i];
    }
}
class Txc1 : public cSimpleModule
{
  private:
    int hop_count = 0; // Đếm số hop từ current bridge tới root bridge
    int designated_port[10]; // cổng designated_port là số thứ tự của port nối tới designated bridge
    int root_port = -1;
    int root_id;// root_id là ID của bridge mà current bridge đang tin là root
    int to_root_id = -1;
    int block_ports[10];
    /* block ports là một danh sách các ID của port mà current bridge không được forward meassge theo
    để tránh tạo loop */
  protected:
    virtual SpanTree *generateMessage(); // Hàm khởi tạo một bản tin configuration
    virtual void forwardMessage(SpanTree *msg, int out); // Hàm giúp bridge forward bản tin
    virtual void initialize() override;  // Hàm khởi tạo mạng và các bridge trong mạng
    virtual void handleMessage(cMessage *msg) override;
    /* Hàm dùng để xử lý bản tin mà bridge nhận được, hầu như tất cả các quy trình cập nhật cho bridge
     đều diễn ra trong hàm này*/
    virtual bool compareID(SpanTree *msg);
    /*So sánh id đầu vào với root id lưu trong object và tự cập nhật các thông số:
     root_id, hop_count, designated_port, block_ports
     Cần trả về giá trị boolean có forward measseage không*/
};

Define_Module(Txc1);

void Txc1::initialize()
{
   // Boot the process scheduling the initial message as a self-message.
    this->root_id = getIndex();
    printf("Initialization here %d", this->root_id);
    printf("\n");
    for(int i = 0; i<10; i++)
    {
        this->block_ports[i] = -1;
        this->designated_port[i] = -1;
    }
//    char msgname[20];
//    sprintf(msgname, "tic-%d", this->root_id);
    // Hàm getName() sẽ trả về ID của Bridge
    SpanTree *msg = generateMessage();
    int n = gateSize("ports");
    printf("gate size: %d",n);
//    int outGateBaseId = gateBaseId("ports$o");
//    printf("outGateBaseId: %d", outGateBaseId);
    for (int i = 0; i < n; i++)
        send(i==n-1 ? msg : msg->dup(),"ports$o",i);
    printf("Done initialize!");
    int delta = 10;
    // Đoạn code này dùng để tự gửi cho sub-module một self-message
    // khi đó, module sẽ check, nếu message tới là một self-message thì cần kiểm tra liệu mình có phải là
    // root không, nếu là root thì sẽ tiếp tục generate message và forward ra tất cả các port
    SpanTree *msg_not_forward = new SpanTree("self_mess");
    scheduleAt(simTime() + delta, msg_not_forward);
    scheduleAt(simTime() + delta*2, msg_not_forward->dup());
    scheduleAt(simTime() + delta*3, msg_not_forward->dup());
}

void Txc1::handleMessage(cMessage *msg)
{
// Kiểm tra ở đây, nếu đủ điều kiện thì sẽ dừng không cho bridge forward nữa
    SpanTree *ttmsg = check_and_cast<SpanTree *>(msg);
//    bool forward = true;
    bool self_mess = (strcmp(ttmsg->getName(), "self_mess") == 0);
//    printf("self_mess %s", self_mess?"true":"false");
    bool forward = false;
    if (self_mess == false)
    {
        forward = this->compareID(ttmsg);
        printf("Done comparing!\n");
    }
//    printf("root now is %d\n", this->root_id);
    int src_id = getIndex();
    bool is_root = false;
//    printf("Current tic %d\n", src_id);
//    printf("Current root %d\n", this->root_id);
    if (src_id == this->root_id) is_root = true;
//    printf("%s", is_root?"true":"false");
    int count_send = 0;
    int n = gateSize("ports");

    if (is_root == false)
    {
    for (int i = 0; i <n; i++)
    {
//        int outGateId = gateBaseId("ports$o") + i;
        if (i == this->root_port)
        {
            cDisplayString& connDispStr = gate("ports$o", i)->getDisplayString();
            connDispStr.parse("ls=black");
            continue;
        }
        else
        {
            bool is_designated_port = false;
            bool is_block_port = false;
            for (int j = 0; j<10; j++)
            {
                if (this->designated_port[j] != -1)
                {
                    if (i == this->designated_port[j])
                    {
                        is_designated_port = true;
                        break;
                    }
                }
                if (this->block_ports[j] != -1)
                {
                    if (i == this->block_ports[j])
                    {
                        is_block_port = true;
                        break;
                    }
                }
            }
            if (is_designated_port == false or is_block_port == true)
            {
                cDisplayString& connDispStr = gate("ports$o", i)->getDisplayString();
                connDispStr.parse("ls=red");
            }
            else
            {
                cDisplayString& connDispStr = gate("ports$o", i)->getDisplayString();
                connDispStr.parse("ls=black");
            }
        }
    }
    }
    if (forward == true && is_root == false && self_mess == false)
    {
        for (int i = 0; i<n; i++)
        {
           bool send = true;
//           int outGateId = gateBaseId("ports$o") + i;
           for(int j = 0; j <10; j++)
           {
               if (i == this-> block_ports[j])
               {
                   send = false;
                   break;
               }
           }
           if (send == true)
           {
               count_send ++;
               ttmsg->setSource_id(getIndex());
               ttmsg->setHopCount(this->hop_count);
               ttmsg->setRoot_port(this->root_port);
               ttmsg->setRoot_id(this->root_id);
               forwardMessage(ttmsg->dup(), i);
           }
        }
       if (count_send == 0) delete ttmsg;
    }
//    else if (is_root == true && self_mess == true)
    else if (src_id  == this->root_id)
    {
    delete ttmsg;
    printf("Send out again from %d!", this->root_id);
    SpanTree *newmsg = generateMessage();
//    int outGateBaseId = gateBaseId("ports$o");
    for (int i = 0; i < n; i++)
      send(i==n-1 ? newmsg : newmsg->dup(), "ports$o", i);
    }
}

SpanTree *Txc1::generateMessage()
{
    // Produce source and destination addresses.
    int src_id = getIndex();
    char msgname[20];
    sprintf(msgname, "tic-%d", src_id);
    // Create message object and set source and destination field.
    SpanTree *msg = new SpanTree(msgname);
    msg->setSource_id(src_id);
    msg->setRoot_id(this->root_id);
    msg->setHopCount(this->hop_count);
    msg->setRoot_port(this->root_port);
    return msg;
}

void Txc1::forwardMessage(SpanTree *msg, int out)
{
    EV << "Forwarding message " << msg << " on port out[" << out << "]\n";
    send(msg, "ports$o", out);
}

bool Txc1::compareID(SpanTree *msg)
{
    printf("come to compareID in %d\n", getIndex());
    int mess_root_id = msg->getRoot_id();
    printf("geting mess_root_id %d\n",msg->getRoot_id());

    int mess_source_id = msg->getSource_id();
    printf("geting mess_source_id %d\n",msg->getSource_id());

    int current_source_id = getIndex();
    int arrivalgateid = msg->getArrivalGate()->getIndex();
    int sendergateid = msg->getSenderGate()->getIndex();
    int n = gateSize("ports");

// Truong hop 1: Root id nhan duoc be hon: cap nhat root id, hop count va root port
    if (this->root_id > mess_root_id)
    {
        this->root_id =  mess_root_id;
        this->to_root_id =  mess_source_id;
        printf("update root id to %d \n", this->root_id);
        this->hop_count = msg->getHopCount()+1;
        this->root_port = arrivalgateid;
        printf("update root port to: %d\n", arrivalgateid);
        for (int i = 0; i < 10; i++)
        {
           if (this->block_ports[i] == arrivalgateid)
           {
              this->block_ports[i] = -1;
           }
        }
        return true;
    }
    // Truong hop 2: Root id nhan duoc lon hon: khong forward meassage nay
    else if (this->root_id < mess_root_id) return false;
    // Truong hop 3: Nguoi gui va nguoi nhan cung chung root id
    else if(this->root_id == mess_root_id)
    {
        // Truong hop 1: Thong qua nguoi gui, nguoi nhan den root nhanh hon: cap nhat root va forward
        if (msg->getHopCount()+1 < this->hop_count)
        {
            this->hop_count = msg->getHopCount()+1;
            this->root_port = arrivalgateid;
            this->to_root_id =  mess_source_id;
            printf("Update to_root_id %d",mess_source_id);
            for (int i = 0; i < 10; i++)
            {
               if (this->block_ports[i] == arrivalgateid)
               {
                  this->block_ports[i] = -1;
               }
            }
            return true;
        }
        // Truong hop 2: Ca 2 cung chung hop count den root nhung thong qua nguoi gui thi co id be hon: cap nhat root va forward
        else if (msg->getHopCount()+1 == this->hop_count && this->to_root_id > mess_source_id)
        {
            this->hop_count = msg->getHopCount()+1;
            this->root_port = arrivalgateid;
            this->to_root_id =  mess_source_id;
            printf("Update to_root_id %d",mess_source_id);
            for (int i = 0; i < 10; i++)
            {
               if (this->block_ports[i] == arrivalgateid)
               {
                  this->block_ports[i] = -1;
               }
            }
            return true;
        }
        // Vay ta da xong tat ca cac truong hop de cap nhat root id va root port
    }
    // Tiep theo, ta can cap nhat designate port, chi cap nhat khi ma root port va root id da on dinh
    if (this->root_id == mess_root_id)
    {
        printf("Sender root port %d\n", msg->getRoot_port());
        printf("Sender port id %d\n", sendergateid);
        //Truong hop 1: Port gui tin la root port: cap nhat port nhan tin thanh designated, khong can forward tin nhan
        if (msg->getRoot_port() == sendergateid)
        {
            for (int i = 0; i < 10; i++)
            {
                if (this->designated_port[i] == -1)
                {
                    this->designated_port[i] = arrivalgateid;
                    break;
                }
                if (this->block_ports[i] == arrivalgateid)
                {
                    this->block_ports[i] = -1;
                }
            }
            return true;
        }
        // Truong hop 2: Bridge gui tin la bridge doi thu
          // Truong hop 1: Bridge nhan gan root hon: ta cap nhat port nhan thanh designated port
        else if (msg->getHopCount() > this->hop_count)
        {
            for (int i = 0; i < 10; i++)
            {
                if (this->designated_port[i] == -1)
                {
                    this->designated_port[i] = arrivalgateid;
                    break;
                 }
            }
            return true;
        }
        // Truong hop 2: Ca 2 co hop count toi root bang nhau, ta can su dung id de giai quyet
        else if (msg->getHopCount() == this->hop_count && getIndex() < mess_source_id)
        {
            for (int i = 0; i < 10; i++)
            {
                if (this->designated_port[i] == -1)
                {
                    this->designated_port[i] = arrivalgateid;
                    break;
                 }
            }
            return true;
        }
        // Truong hop 3: bridge nhan kem hon, port nhan can tro thanh block port
        else
        {
            for (int i = 0; i < 10; i++)
            {
                if (this->designated_port[i] == arrivalgateid)
                {
                    this->designated_port[i] = -1;
                 }
                if (this->block_ports[i] == -1)
                {
                    this->block_ports[i] = arrivalgateid;
                }
            }
            return false;
        }
    }
}

