#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

// Coefficients, which are used in rank calculating formula
constexpr int DISTANCE_COEFFICIENT = 5000;
constexpr int TIME_COEFFICIENT = 1;

// Define file names here
const string DONATION_FILE_NAME = "donation.txt";
const string ORDERS_FILE_NAME = "orders.txt";
const string RESULT_DONATION_FILE_NAME = "result_donation.txt";
const string RESULT_ORDERS_FILE_NAME = "result_orders.txt";


struct Location {
    // Place coordinates, could be anything you want km, m, longitude, latitude and soon, but need to change `DISTANCE_COEFFICIENT` to make rank calculating balances with time
    double x;
    double y;

    Location(const double &x, const double &y): x(x), y(y) {
    }
};

// General structure for donation and order, because they mostly have same fields
struct Data {
    unsigned int id, productId, quantity;
    int timestamp;
    Location location;

    Data(const unsigned int &id, const unsigned int &productId, const unsigned int &quantity,
         const int &timestamp,
         const double &x,
         const double &y): id(id),
                           productId(productId),
                           quantity(quantity),
                           timestamp(timestamp),
                           location(x, y) {
    }
};

// Donation struct
struct Donation : Data {
    using Data::Data;
};

/* Order struct.
 * Timestamp order param should be less, than Donation timestamp param, because
 */
struct Order : Data {
    int rank = -1; // field for rank calculation
    using Data::Data;
};


/** @brief Takes the donation file name and reads the donation `Data line`
  * @param filename Donation file name
  * @returns Donation*  Pointer to new Donation struct
  * @throws runtime_error Could not open file or file contains wrongly formatted line
  */
Donation *readDonation(const string &filename) {
    ifstream file("donation.txt");
    if (!file.is_open()) throw runtime_error("Unable to open file: " + filename + "\n");

    string line;
    getline(file, line);
    istringstream ss(line);

    // data struct fields
    unsigned int id, productId, quantity;
    double latitude, longitude;
    int timestamp;

    if (!(ss >> id >> productId >> quantity >> latitude >> longitude >> timestamp)) {
        throw runtime_error("Wrongly formated `Data line`. \n "
                            "Expected format: `{id:unsigned int} {productId:unsigned int} {quantity:unsigned int} {x:double} {y:double} {timestamp:int}` \n"
                            "But got " + line + "\n");
    }

    if (getline(file, line)) cout << "Detected extra lines in donation file. They are ignored" << endl;

    auto *ptr = new Donation(id, productId, quantity, timestamp, latitude, longitude);
    return ptr;
}

/** @brief Takes the orders file name and reads order `Data lines`
  * @param filename Order file name
  * @param prodId If the order has a different `prodId`, it will be ignored
  * @returns vector<Order *>  Vector of the Order* pointers
  * @throws runtime_error Could not open file or file contains wrongly formatted line
  */
vector<Order *> readOrders(const string &filename, const unsigned int &prodId) {
    vector<Order *> orders;
    ifstream file(filename);

    if (!file.is_open()) throw runtime_error("Unable to open file: " + filename + "\n");

    string line;
    unsigned int id, productId, quantity;
    double latitude, longitude;
    int timestamp;

    while (getline(file, line)) {
        istringstream ss(line);
        if (!(ss >> id >> productId >> quantity >> latitude >> longitude >> timestamp)) {
            throw runtime_error("Wrongly formated `Data line`. \n "
                                "Expected format: `{id:unsigned int} {productId:unsigned int} {quantity:unsigned int} {x:double} {y:double} {timestamp:int}` \n"
                                "But got " + line + "\n");
        }
        // An order that contains another product cannot be fulfilled, so it will be ignored
        if (prodId == productId) orders.push_back(new Order(id, productId, quantity, timestamp, latitude, longitude));
    }

    return orders;
}

/** @brief Calculate distance between to Locations
  * @param loc1 In program context - Donation location
  * @param loc2 In program context - Order location
  * @returns Distance between two locations
  */
double calculateDistance(const Location &loc1, const Location &loc2) {
    return (sqrt(pow(loc1.x - loc2.x, 2) + pow(loc1.y - loc2.y, 2)));
}

/** @brief Calculate time difference
  * @param time1 In program context - Time, when donation was placed (basically, should be more than time2)
  * @param time2 In program context - Time, when order was placed (basically, should be less than time2)
  * @returns Time difference
  */
int timeDif(const int &time1, const int &time2) {
    if (time1 < time2)
        cout << "Order timestamp is greater that donation timestamp. Program behaviour could be unexpected!" << endl;
    return time1 - time2;
}

/** @brief Calculates ranks to all Order in orders vector.
  * @param orders Order* vector, expected, that all orders already has same product as Donation
  * @param donation Donation*
  * @returns Void, but modifies Order in orders.
  */
void calculateRanks(const vector<Order *> &orders, const Donation *donation) {
    for (Order *order: orders) {
        const double distance = calculateDistance(order->location, donation->location);
        const int tDif = timeDif(donation->timestamp, order->timestamp);
        order->rank = static_cast<int>(round (1 / distance * DISTANCE_COEFFICIENT) ) + tDif * TIME_COEFFICIENT;
    }
}

/** @brief Saves results to files
  * @param orders Order* vector, Orders which was processed (product quantity changed)
  * @param donation Donation* split donation
  * @param resultDonationFileName File name where will we saved the remaining Donation
  * @param resultOrdersFileName File name where will be saved processed Orders
  * @returns Void, but creates 2 files
  */

void saveResults(const vector<Order *> &orders, const Donation *donation, const string &resultDonationFileName,
                 const string &resultOrdersFileName) {
    ofstream ordersFile(resultOrdersFileName);
    if (orders.empty()) ordersFile << "No orders match to donation.\n";
    for (const Order *order: orders) {
        ordersFile << order->id << ' ' << ' ' << order->quantity << endl;
    }

    ofstream remainingDonationFile(resultDonationFileName);
    remainingDonationFile << donation->id << " " << donation->quantity << endl;
}

/**
 * @brief This is the function that is described in the document. Takes two files (names) and optimally splits donation to orders
 * @param donationFileName File name which contains donation data
 * @param ordersFileName File name which contain orders data
 * @param resultDonationFileName File name where will we saved the remaining Donation
 * @param resultOrdersFileName File name where will be saved processed Orders
 */
void splitDonationToOrders(const string &donationFileName, const string &ordersFileName,
                           const string &resultDonationFileName, const string &resultOrdersFileName) {
    // 1. Step - Reading data from files
    Donation *donation = readDonation(donationFileName);
    vector<Order *> orders = readOrders(ordersFileName, donation->productId);

    // 2. Step - calculate order ranks
    calculateRanks(orders, donation);

    // 3. Step - sort orders by rank
    sort(orders.begin(), orders.end(), [](const Order *a, const Order *b) {
        return a->rank > b->rank;
    });

    // 4. Step - split Donation product to orders
    int lastIndex = 1;
    for (Order *order: orders) {
        if (order->quantity < donation->quantity) {
            donation->quantity -= order->quantity;
            order->quantity = 0;
        } else {
            order->quantity -= donation->quantity;
            donation->quantity = 0;
            break;
        }
        lastIndex++;
    }
    orders.erase(orders.begin() + lastIndex, orders.end());

    // 5. Step - save results
    saveResults(orders, donation, resultDonationFileName, resultOrdersFileName);

    delete donation;
}

int main() {
    splitDonationToOrders(DONATION_FILE_NAME, ORDERS_FILE_NAME, RESULT_DONATION_FILE_NAME, RESULT_ORDERS_FILE_NAME);

    cout << "Order processing is completed!" << std::endl;
    return 0;
}
