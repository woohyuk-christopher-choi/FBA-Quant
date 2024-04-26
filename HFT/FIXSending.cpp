#include "quickfix/Application.h"
#include "quickfix/SocketInitiator.h"
#include "quickfix/SessionSettings.h"
#include "quickfix/FileStore.h"
#include "quickfix/FileLog.h"
#include "quickfix/fix42/NewOrderSingle.h"
#include "quickfix/MessageCracker.h"

class MyTradingApplication : public FIX::Application, public FIX::MessageCracker
{
protected:
    void onCreate(const FIX::SessionID&) override {}
    void onLogon(const FIX::SessionID& sessionID) override {}
    void onLogout(const FIX::SessionID& sessionID) override {}
    void toAdmin(FIX::Message&, const FIX::SessionID&) override {}
    void fromAdmin(const FIX::Message&, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override {}
    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::DoNotSend) override {}
    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override {
        crack(message, sessionID);
    }

    void onMessage(const FIX42::NewOrderSingle& message, const FIX::SessionID&) {
        FIX::Symbol symbol;
        FIX::Side side;
        FIX::OrdType ordType;
        FIX::OrderQty orderQty;
        FIX::Price price;
    
        message.get(symbol);
        message.get(side);
        message.get(ordType);
        message.get(orderQty);
        message.get(price);
    
        std::string orderType = ordType == FIX::OrdType_MARKET ? "Market" : "Limit";
        std::string orderSide = side == FIX::Side_BUY ? "Buy" : "Sell";
    
        
        std::cout << "Received " << orderSide << " Order - "
                  << "Symbol: " << symbol << ", "
                  << "Quantity: " << orderQty << ", "
                  << "Price: $" << price << ", "
                  << "Type: " << orderType << std::endl;
    

        if (shouldExecuteOrder(price, orderQty)) {
            executeOrder(symbol, side, orderQty, price);
        } else {
            std::cout << "Order not executed: market conditions not met." << std::endl;
        }
    }

    bool shouldExecuteOrder(const FIX::Price& price, const FIX::OrderQty& qty) {
        double marketPrice = getMarketPrice(); 
        return price <= marketPrice && qty >= 100; 
    }

    void executeOrder(const FIX::Symbol& symbol, const FIX::Side& side, const FIX::OrderQty& qty, const FIX::Price& price) {
        std::string orderSide = side == FIX::Side_BUY ? "Buy" : "Sell";
        std::cout << "Executing " << orderSide << " Order: "
                  << qty << " shares of " << symbol << " at $" << price << std::endl;
        processOrderExecution(symbol, qty, price);
    }

    void processOrderExecution(const FIX::Symbol& symbol, const FIX::OrderQty& qty, const FIX::Price& price) {
        std::cout << "Order executed for " << qty << " shares of " << symbol << " at $" << price << std::endl;
    }

public:
    void sendOrder() {
        FIX::Session* session = FIX::Session::lookupSession(mySessionID);
        if(session) {
            FIX42::NewOrderSingle order(
                FIX::ClOrdID("OrderID"),
                FIX::HandlInst('1'),
                FIX::Symbol("AAPL"),
                FIX::Side(FIX::Side_BUY),
                FIX::TransactTime(),
                FIX::OrdType(FIX::OrdType_MARKET));

            order.set(FIX::OrderQty(100));
            order.set(FIX::Price(150.50));

            FIX::Session::sendToTarget(order);
        }
    }
};

int main() {
    FIX::SessionSettings settings("config.cfg");
    MyTradingApplication application;
    FIX::FileStoreFactory storeFactory(settings);
    FIX::FileLogFactory logFactory(settings);
    FIX::SocketInitiator initiator(application, storeFactory, settings, logFactory);

    initiator.start();
    application.sendOrder();
    initiator.stop();

    return 0;
}
