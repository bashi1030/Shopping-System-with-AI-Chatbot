#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <curl/curl.h>
#include <json.hpp>


using json = nlohmann::json;

#define RESET_COLOR "\033[0m"
#define RED_COLOR "\033[31m"
#define GREEN_COLOR "\033[32m"
#define YELLOW_COLOR "\033[33m"
#define BLUE_COLOR "\033[34m"
#define MAGENTA_COLOR "\033[35m"
#define CYAN_COLOR "\033[36m"
#define WHITE_COLOR "\033[37m"
#define BOLD_TEXT "\033[1m"

using namespace std;

const string GEMINI_API_KEY="AIzaSyDElMh6cqCVTjoY_ReEgDgP4VjrR1MdY_c";

void saveAllData();
void loadAllData();
void clearScreen();
void safeCinClear();

class Product
{
    string name;
    double price;
    int quantity;
    string sellerUsername;
    string category;

public:
    Product(string n="", double p=0.0, int q=0,string s_uname="",string cat = "")
        : name(n), price(p), quantity(q), sellerUsername(s_uname), category(cat) {}

    void display() const
{
        cout<<"Name: "<<name<<", Category: "<<category<<", Price: $"<<fixed<<setprecision(2)<<price;
        cout<<", Quantity: "<<quantity<<", Seller: "<<sellerUsername<<endl;
}

    string getName() const
{
        return name;
}
    double getPrice() const
{
        return price;
}
    int getQuantity() const
{
        return quantity;
}
    string getSellerUsername() const
{
        return sellerUsername;
}
    string getCategory() const
{
        return category;
}

    void setName(const string& n)
{
        name=n;
}
    void setPrice(double p)
{
        price=p;
}
    void setQuantity(int q)
{
        quantity=q;
}
    void setCategory(const string& c)
{
        category=c;
}


    string toString() const
{
    try
{
        if (name.empty() || sellerUsername.empty() || category.empty())
{
                cout<<RED_COLOR<<"Error: Empty name, seller, or category."<<RESET_COLOR<<endl;
                return "";
}
            if (price<0 || quantity<0)
{
                cout<<RED_COLOR<<"Error  negative price or quantity."<<RESET_COLOR<<endl;
                return "";
}

            string temname=name;
            string tempseller=sellerUsername;
            string escapedCategory=category;


            for (int i=0; i<escapedCategory.length(); i++)
                if (escapedCategory[i]==',') escapedCategory[i]=';';


            stringstream ss;
            ss<<temname<<","<<fixed<<setprecision(2)<<price<<","<<quantity<<","<<tempseller<<","<<escapedCategory;
            string result=ss.str();
            if (ss.fail())
                throw "Stream error";
            return result;
}
        catch (...)
{
            cout<<RED_COLOR<<"Error: Failed to make string of  product."<<RESET_COLOR<<endl;
            return "";
}
}



    static Product fromString(const string& line, bool& success)
{
        try
{
            stringstream ss(line);
            string name, priceStr,quantityStr,sellerUsername,category;

            if (!getline(ss, name, ',') || name.empty() || !getline(ss, priceStr, ',') ||
                    priceStr.empty() || !getline(ss, quantityStr, ',') || quantityStr.empty() ||
                    !getline(ss, sellerUsername, ',') || sellerUsername.empty() ||
                    !getline(ss, category, ',') || category.empty())
{
                cout<<RED_COLOR<<"Error: Invalid product format in line: '"<<line<<"'"<<RESET_COLOR<<endl;
                success=false;
                return Product();
}

            for (int i=0; i<category.length(); i++)
{
                if (category[i]==';') category[i]=',';
}



            stringstream priceSS(priceStr);
            stringstream quantitySS(quantityStr);

            double price;
            int quantity;
            priceSS>>price;
            quantitySS>>quantity;
            if (priceSS.fail() || quantitySS.fail())
                throw "Invalid number format";
            if (price<0 || quantity<0)
{
                cout<<RED_COLOR<<"Error: Negative price or quantity in line: '"<<line<<"'"<<RESET_COLOR<<endl;
                success=false;
                return Product();
}

            success=true;
            return Product(name, price, quantity, sellerUsername, category);
}
        catch (...)
{
            cout<<RED_COLOR<<"Error: Failed to parse product in line: '"<<line<<"'"<<RESET_COLOR<<endl;
            success=false;
            return Product();
}
}
};

vector<Product>allProducts;

class User;
class Seller;
class Buyer;

static size_t WriteCallback(char* contents, size_t size, size_t nmemb, string* s)
{
    size_t newLength = size * nmemb;
    try
{
        s->append(contents, newLength);
}
    catch (...)
{
        cout<<RED_COLOR<<"Error: Memory allocation failed in WriteCallback."<<RESET_COLOR<<endl;
        return 0;
}
    return newLength;
}

string getGeminiChatResponse(const string& userInput, const string& apiKey)
{
    CURL* curl;
    CURLcode res;
    string readBuffer;
    string aiResponse="Error: Could not get response from Gemini AI.";

    string context="You are a customer support chatbot for a shopping system. Strictly answer questions related to available products, shopping, cart, payments, and account management within a shopping system context. Do NOT answer unrelated questions or act as a general AI. \n\n";

    if (!allProducts.empty())
{
        context+="Currently available products:\n";
        for (int i=0; i<allProducts.size(); i++)
{
            stringstream priceStream;
            priceStream<<fixed<<setprecision(2)<<allProducts[i].getPrice();
            context+=allProducts[i].getName()+" (Category: "+ allProducts[i].getCategory() + ", Price: $" + priceStream.str() + ", Quantity: " + to_string(allProducts[i].getQuantity()) + ", Seller: " + allProducts[i].getSellerUsername() + ")\n";
}
        context+="\n";
}
    else
{
        context+="No products are currently listed in the system.\n\n";
}

    string fullUserInput=context+"User Query: "+userInput;

    string geminiApiUrl="https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + apiKey;
    curl=curl_easy_init();
    if (curl)
{


        struct curl_slist* headers=nullptr;
        headers=curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER,headers);

        json requestPayload;
        json part;
        part["text"]=fullUserInput;
        json content;
        content["parts"]=json::array({part});
        requestPayload["contents"]=json::array({content});

        string payloadStr=requestPayload.dump();

        curl_easy_setopt(curl,CURLOPT_URL, geminiApiUrl.c_str());
        curl_easy_setopt(curl,CURLOPT_POSTFIELDS,payloadStr.c_str());
        curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,&readBuffer);
        curl_easy_setopt(curl,CURLOPT_USERAGENT,"my-cpp-gemini-chatbot");
        curl_easy_setopt(curl,CURLOPT_TIMEOUT_MS,45000L);

        string caBundlePath="C:\\Users\\Shabby\\Desktop\\Final Project Submission\\Project\\cacert.pem";
        ifstream caFile(caBundlePath);
        if (!caFile.is_open())
{
            aiResponse="Error: CA certificate file 'cacert.pem' not found.";
            cerr<<RED_COLOR<<aiResponse<<RESET_COLOR<<endl;
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return aiResponse;
}
        caFile.close();
        curl_easy_setopt(curl, CURLOPT_CAINFO, caBundlePath.c_str());

        res=curl_easy_perform(curl);

        long http_code=0;
        if (res==CURLE_OK)
{
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
}

        if (res!=CURLE_OK)
{
            aiResponse="curl_easy_perform() failed: "+string(curl_easy_strerror(res));
}
        else if (http_code!=200)
{
            aiResponse="Gemini API Error: HTTP status "+to_string(http_code)+"\nResponse: "+readBuffer;
}
        else
{
            try
{
                json jsonResponse=json::parse(readBuffer);
                if (jsonResponse.contains("candidates") && jsonResponse["candidates"].is_array() && !jsonResponse["candidates"].empty())
{
                    json& firstCandidate = jsonResponse["candidates"][0];
                    if (firstCandidate.contains("content") && firstCandidate["content"].contains("parts") &&
                            !firstCandidate["content"]["parts"].empty() &&
                            firstCandidate["content"]["parts"][0].contains("text"))
{
                        aiResponse=firstCandidate["content"]["parts"][0]["text"].get<string>();
}
                    else
{
                        aiResponse="Error: 'text' not found in Gemini response.";
}
}
                else
{
                    aiResponse="Error: Invalid Gemini response structure.";
}
}
            catch (...)
{
                aiResponse="Error: Failed to parse Gemini JSON response.";
}

}

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
}
    else
{
        aiResponse = "Error: CURL initialization failed.";
}

    return aiResponse;
}

int runChatbot()
{
    if (GEMINI_API_KEY.empty())
{
        cout<<RED_COLOR<<"ERROR: Gemini API key not provided."<<RESET_COLOR<<endl;
        return 1;
}

    clearScreen();
    cout<<BOLD_TEXT<<CYAN_COLOR<<":::::  Customer Support Chatbot :::::"<<RESET_COLOR<<endl;
    cout<<"Customer support Chatbot: Hello! How can I help you today? Type 'exit' to return to the main menu.\n";

    string userInput;
    safeCinClear();
    while (true)
{
        cout<<BOLD_TEXT<<GREEN_COLOR<<"\nYou: "<<RESET_COLOR;
        getline(cin, userInput);

        if (userInput=="exit" || userInput=="quit")
{
            cout<<"Customer support Chatbot: Goodbye! Returning to main menu.\n";
            break;
}

        if (userInput.empty())
            continue;

        cout<<"Chatbot: Thinking...\n";
        string response =getGeminiChatResponse(userInput, GEMINI_API_KEY);
        cout<<BOLD_TEXT<<YELLOW_COLOR<<"Gemini AI Chatbot: "<<response<<RESET_COLOR<<"\n";
}

    return 0;
}

class User
{
protected:
    string username;
    string password;
    double balance;
    string type;

public:
    User(string u,string p,string t, double bal=0.0): username(u), password(p), type(t), balance(bal) {}

    virtual ~User(){};

    virtual void menu()=0;

    string getUsername() const
{
        return username;
}
    string getPassword() const
{
        return password;
}
    string getType() const
{
        return type;
}
    double getBalance() const
{
        return balance;
}
    void deposit(double amount)
{
        balance+=amount;
}
    void withdraw(double amount)
{
        balance -= amount;
}

    virtual string toString() const
{
        stringstream ss;
        ss<<username<<","<<password<<","<<fixed<<setprecision(2)<<balance<<","<<type;
        return ss.str();
}
};

vector<User*>allUsers;

class Seller:public User
{
public:
    Seller(string u, string p, double bal = 0.0): User(u, p, "Seller", bal) {}

    void addProduct()
{
        clearScreen();
        string name;
        double price;
        int quantity;
        string category;

        cout<<BOLD_TEXT<<BLUE_COLOR<<"\3\3\3\3\3 Add New Product \3\3\3\3\3"<<RESET_COLOR<<endl;
        cout<<"Enter product name: ";
        safeCinClear();
        getline(cin, name);

        cout<<"Enter category (e.g., Electronics,Shoes, Books, Clothes): ";
        getline(cin, category);

        cout<<"Enter price: $";
        while (!(cin>>price) || price<0)
{
            cout<<RED_COLOR<<"Invalid input. Please enter a valid positive number for price: $"<<RESET_COLOR;
            safeCinClear();
}

        cout<<"Enter quantity: ";
        while (!(cin >> quantity) || quantity < 0)
{
            cout<<RED_COLOR<<"Invalid input. Please enter a valid positive whole number for quantity: "<<RESET_COLOR;
            safeCinClear();
}

        allProducts.push_back(Product(name, price, quantity, username, category));
        cout<<GREEN_COLOR<<"Product added successfully!"<<RESET_COLOR << endl;
        saveAllData();
        cout<<"Press Enter to continue...";
        safeCinClear();
        cin.get();
}

    void viewProducts() const
{
        clearScreen();
        cout<<BOLD_TEXT<<BLUE_COLOR<<"::::: Products You Listed :::::"<<RESET_COLOR<<endl;
        bool found=false;
        for (int i=0; i<allProducts.size(); i++)
{
            if (allProducts[i].getSellerUsername()==username)
{
                cout<<(i+1)<<". ";
                allProducts[i].display();
                found=true;
}
}
        if (!found)
{
            cout<<"You haven't listed any products yet."<<endl;
}
        cout<<"Press Enter to continue...";
        safeCinClear();
        cin.get();
}

    void editProduct()
{
        clearScreen();
        cout<<BOLD_TEXT<<BLUE_COLOR<<"\1\1\1\1 Edit Product \1\1\1\1 "<<RESET_COLOR<<endl;

        vector<int> sellerProducts;
        cout<<BOLD_TEXT<<CYAN_COLOR<<"\4\4\4\4 Your Products \4\4\4\4"<<RESET_COLOR<<endl;
        bool found=false;
        for (int i=0; i<allProducts.size(); i++)
{
            if (allProducts[i].getSellerUsername()==username)
{
                cout<<(i+1)<<". ";
                allProducts[i].display();
                sellerProducts.push_back(i);
                found=true;
}
}

        if (!found)
{
            cout<<"You haven't listed any products yet."<<endl;
            cout<<"Press Enter to continue...";
            safeCinClear();
            cin.get();
            return;
}

        cout<<"\nEnter the number of the product to edit: ";
        int productNumber;
        while (!(cin>>productNumber) || productNumber<=0 || productNumber>allProducts.size() ||
                find(sellerProducts.begin(), sellerProducts.end(), productNumber-1) == sellerProducts.end())
{
            cout<<RED_COLOR<<"Invalid product number. Please enter a valid number: "<<RESET_COLOR;
            safeCinClear();
}

        Product& selectedProduct=allProducts[productNumber - 1];
        cout<<BOLD_TEXT<<CYAN_COLOR<<"Current Product Details:"<<RESET_COLOR<<endl;
        selectedProduct.display();

        cout<<"\nEnter new product name (press Enter to keep '"<<selectedProduct.getName()<<"'): ";
        safeCinClear();
        string newName;
        getline(cin, newName);
        if (!newName.empty())
{
            selectedProduct.setName(newName);
}

        cout<<"Enter new category (press Enter to keep '"<<selectedProduct.getCategory()<<"'): ";
        string newCategory;
        getline(cin, newCategory);
        if (!newCategory.empty())
{
            selectedProduct.setCategory(newCategory);
}

        cout<<"Enter new price (enter -1 to keep $"<<fixed<<setprecision(2)<<selectedProduct.getPrice()<<"): $";
        double newPrice;
        while (!(cin>>newPrice) || (newPrice < 0 && newPrice != -1))
{
            cout<<RED_COLOR<<"Invalid input. Please enter a valid positive number or -1 to keep current price: $"<<RESET_COLOR;
            safeCinClear();
}
        if (newPrice != -1)
{
            selectedProduct.setPrice(newPrice);
}

        cout<<"Enter new quantity (enter -1 to keep "<<selectedProduct.getQuantity()<<"): ";
        int newQuantity;
        while (!(cin>>newQuantity) || (newQuantity < 0 && newQuantity != -1))
{
            cout<<RED_COLOR<<"Invalid input. Please enter a valid non-negative number or -1 to keep current quantity: "<<RESET_COLOR;
            safeCinClear();
}
        if (newQuantity != -1)
{
            selectedProduct.setQuantity(newQuantity);
}

        cout<<GREEN_COLOR<<"Product updated successfully!"<<RESET_COLOR<<endl;
        saveAllData();
        cout<<"Press Enter to continue...";
        safeCinClear();
        cin.get();
}

    void menu()
{
        int choice;
        do
{
            clearScreen();
            cout<<BOLD_TEXT<<MAGENTA_COLOR<<"\1\1\1\Seller Menu \1\1\1"<<RESET_COLOR<<endl;
            cout<<BOLD_TEXT<<"Welcome, "<<username<<"! Your Balance: $"<<balance<<RESET_COLOR<<endl;
            cout<<"1. Add Product\n";
            cout<<"2. View Your Products\n";
            cout<<"3. Edit Product\n";
            cout<<"0. Logout\n";
            cout<<BOLD_TEXT<<YELLOW_COLOR<<"Choice: "<<RESET_COLOR;

            while (!(cin >> choice))
{
                cout<<RED_COLOR<<"Invalid input. Please enter a number: "<<RESET_COLOR;
                safeCinClear();
}

            switch (choice)
{
            case 1:
                addProduct();
                break;
            case 2:
                viewProducts();
                break;
            case 3:
                editProduct();
                break;
            case 0:
                cout<<"Logging out...\n";
                break;
            default:
                cout<<RED_COLOR<<"Invalid choice"<<RESET_COLOR<<endl;
                cout<<"Press Enter to continue...";
                safeCinClear();
                cin.get();
}
}
        while (choice!=0);
}
};

class Buyer : public User
{
    vector<Product>cart;

public:
    Buyer(string u, string p,double bal=0.0) : User(u,p, "Buyer",bal) {}

    string toString() const
{
        string base=User::toString();
        base+=",[";
        for (int i=0; i<cart.size(); i++)
{
            base+=cart[i].getName()+":"+to_string(cart[i].getQuantity());//to string is global function in string lab
            if (i<cart.size()-1)
                base+=";";
}
        base+="]";
        return base;
}

    void loadCartFromString(string cartData)
{
        cart.clear();
        if (cartData.empty() || cartData == "[]")
            return;

        string dataToParse=cartData;
        if (dataToParse.front()=='[' && dataToParse.back()==']')
{
            dataToParse=dataToParse.substr(1, dataToParse.length()-2);
}

        stringstream ss(dataToParse);
        string itemStr;
        while (getline(ss,itemStr,';'))
{
            int colonPos=itemStr.find(':');
            if (colonPos!=string::npos)
{
                string productName=itemStr.substr(0,colonPos);
                string quantityStr=itemStr.substr(colonPos+1);
                try
{
                    stringstream quantitySS(quantityStr);
                    int quantity;
                    quantitySS>>quantity;
                    if (quantitySS.fail())
                        throw "Invalid quantity";

                    bool foundProduct=false;
                    for (int i=0; i<allProducts.size(); i++)
{
                        if (allProducts[i].getName()==productName)
{
                            cart.push_back(Product(productName,allProducts[i].getPrice(),quantity,allProducts[i].getSellerUsername(), allProducts[i].getCategory()));
                            foundProduct=true;
                            break;
}
}
                    if (!foundProduct)
{
                        cart.push_back(Product(productName,0.0,quantity,"Unknown Seller", "Unknown"));
                        cout<<RED_COLOR<<"Warning: Product '"<<productName<<"' not found."<<RESET_COLOR<<endl;
}
}
                catch (...)
{
                    cout<<RED_COLOR<<"Error: Invalid quantity '"<<quantityStr<<"' for product '"<<productName<<"'."<<RESET_COLOR<<endl;
}
}
}
}

    void updateBalance()
{
        clearScreen();
        cout<<BOLD_TEXT<<BLUE_COLOR<<"::::: Update Balance :::::"<<RESET_COLOR<<endl;
        stringstream balanceStream;
        balanceStream<<fixed<<setprecision(2)<<balance;
        cout<<"Current balance: $"<<balanceStream.str()<<endl;
        cout<<"1. Deposit Funds\n";
        cout<<"2. Withdraw Funds\n";
        cout<<"0. Back to Buyer Menu\n";
        cout<<BOLD_TEXT<<YELLOW_COLOR<<"Choice: "<<RESET_COLOR;

        int choice;
        while (!(cin>>choice) || (choice < 0 || choice > 2))
{
            cout<<RED_COLOR<<"Invalid choice. Please enter 0, 1, or 2: "<<RESET_COLOR;
            safeCinClear();
}

        if (choice==0)
{
            return;
}

        double amount;
        cout<<"Enter amount: $";
        while (!(cin>>amount) || amount <= 0)
{
            cout<<RED_COLOR<<"Invalid amount. Please enter a positive number: $"<<RESET_COLOR;
            safeCinClear();
}

        if (choice == 1)
{
            deposit(amount);
            cout<<GREEN_COLOR<<"Deposited $"<<amount<<" successfully! New balance: $"<<balance<<".00"<<RESET_COLOR<<endl;
}
        else if (choice==2)
{
            if (amount <= balance)
{
                withdraw(amount);
                cout<<GREEN_COLOR<<"Withdrew $"<<amount<<" successfully! New balance: $"<<balance<<".00"<<RESET_COLOR<<endl;
}
            else
{
                cout<<RED_COLOR<<"Insufficient funds. Current balance: $"<<balance<<".00"<<RESET_COLOR<<endl;
}
}

        saveAllData();
        cout<<"Press Enter to continue...";
        safeCinClear();
        cin.get();
}

    void displayAllProducts() const
{
        clearScreen();
        cout<<BOLD_TEXT<<CYAN_COLOR<<"::::: All Available Products :::::"<<RESET_COLOR<<endl;
        if (allProducts.empty())
{
            cout<<"No products available."<<endl;
}
        else
{
            for (int i=0; i<allProducts.size(); i++)
{
                cout<<(i+1)<<". ";
                allProducts[i].display();
}
}
        cout<<"Press Enter to continue...";
        safeCinClear();
        cin.get();
}

    void viewProductsByCategory() const
{
        clearScreen();
        cout<<BOLD_TEXT<<BLUE_COLOR<<"::::: Browse by Category :::::"<<RESET_COLOR<<endl;

        vector<string>categories;
        for (int i=0; i<allProducts.size(); i++)
{
            categories.push_back(allProducts[i].getCategory());
}
        sort(categories.begin(), categories.end());
        categories.erase(unique(categories.begin(), categories.end()), categories.end());

        if (categories.empty())
{
            cout<<"No categories available."<<endl;
            cout<<"Press Enter to continue...";
            safeCinClear();
            cin.get();
            return;
}

        cout<<"Available Categories:\n";
        for (int i=0; i<categories.size(); i++)
{
            cout<<(i + 1)<<". " <<categories[i]<<endl;
}
        cout<<"\nEnter the category name to view products : ";
        string chosenCategory;
        safeCinClear();
        getline(cin, chosenCategory);

        cout<<BOLD_TEXT<<CYAN_COLOR<<"\n--- Products in Category: "<<chosenCategory<<" ---"<<RESET_COLOR<<endl;
        bool found=false;
        int count = 0;
        string chosenCategoryLower = chosenCategory;
        for (int i=0; i<chosenCategoryLower.length(); i++)
{
            chosenCategoryLower[i]=tolower(chosenCategoryLower[i]);
}

        for (int i=0; i<allProducts.size(); i++)
{
            string productCategoryLower = allProducts[i].getCategory();
            for (int j=0; j<productCategoryLower.length(); j++)
{
                productCategoryLower[j] = tolower(productCategoryLower[j]);
}

            if (productCategoryLower==chosenCategoryLower)
{
                cout<<(count++)<<". ";
                allProducts[i].display();
                found=true;
}
}

        if (!found)
{
            cout<<"No products found in category '"<<chosenCategory<<"'."<<endl;
}
        cout<<"Press Enter to continue...";
        cin.get();
}



    void browseProductsMenu() const
{
        int choice;
        do
{
            clearScreen();
            cout<<BOLD_TEXT<<CYAN_COLOR<<"::::: Browse Products :::::"<<RESET_COLOR<<endl;
            cout<<"1. View All Products\n";
            cout<<"2. View Products by Category\n";
            cout<<"0. Back to Buyer Menu\n";
            cout<<BOLD_TEXT<<YELLOW_COLOR<<"Choice: "<<RESET_COLOR;

            while (!(cin>>choice))
{
                cout<<RED_COLOR<<"Invalid input. Please enter a number: "<<RESET_COLOR;
                safeCinClear();
}

            switch (choice)
{
            case 1:
                displayAllProducts();
                break;
            case 2:
                viewProductsByCategory();
                break;
            case 0:
                cout<<"Returning to Buyer Menu...\n";
                break;
            default:
                cout<<RED_COLOR<<"Invalid choice"<<RESET_COLOR<<endl;
                cout<<"Press Enter to continue...";
                safeCinClear();
                cin.get();
}
}
        while (choice != 0);
}

    void searchProducts() const
{
        clearScreen();
        string searchTerm;
        cout<<BOLD_TEXT<<BLUE_COLOR<<"::::: Search Products :::::"<<RESET_COLOR<<endl;
        cout<<"Enter product name (or part of it) to search: ";
        safeCinClear();
        getline(cin, searchTerm);

        cout<<BOLD_TEXT<<CYAN_COLOR<<"\n::::: Search Results :::::"<<RESET_COLOR<<endl;
        bool found=false;
        int count=0;
        string searchTermLower = searchTerm;
        for (int i=0; i<searchTermLower.length(); i++)
{
            searchTermLower[i]=tolower(searchTermLower[i]);
}

        for (int i=0; i<allProducts.size(); i++)
{
            string productNameLower = allProducts[i].getName();
            for (int j=0; j<productNameLower.length(); j++)
{
                productNameLower[j] = tolower(productNameLower[j]);
}

            if (productNameLower.find(searchTermLower)!=string::npos)
{
                cout<<(count++)<<". ";
                allProducts[i].display();
                found=true;
}
}

        if (!found)
{
            cout<<"No products found matching your search term."<<endl;
}
        cout<<"Press Enter to continue...";
        cin.get();
}

    void sortProducts()
{
        clearScreen();
        cout<<BOLD_TEXT<<BLUE_COLOR<<"::::: Sort Products by Price :::::"<<RESET_COLOR<<endl;
        cout<<"1. Sort by Price (Low to High)\n";
        cout<<"2. Sort by Price (High to Low)\n";
        cout<<BOLD_TEXT<<YELLOW_COLOR<<"Choice: "<<RESET_COLOR;
        int sortChoice;

        while (!(cin>>sortChoice) || (sortChoice != 1 && sortChoice != 2))
{
            cout<<RED_COLOR<<"Invalid choice. Please enter 1 or 2: "<<RESET_COLOR;
            safeCinClear();
}

        if (sortChoice==1)
{
            sort(allProducts.begin(), allProducts.end(), [](const Product& a, const Product& b)
{
                return a.getPrice() < b.getPrice();
});
            cout<<GREEN_COLOR<<"Products sorted by price (Low to High)."<<RESET_COLOR<<endl;
}
        else
{
            sort(allProducts.begin(), allProducts.end(), [](const Product& a, const Product& b)
{
                return a.getPrice()>b.getPrice();
});
            cout<<GREEN_COLOR<<"Products sorted by price (High to Low)."<<RESET_COLOR<<endl;
}

        cout<<BOLD_TEXT<<CYAN_COLOR<<"\n::::: Sorted Products :::::"<<RESET_COLOR<<endl;
        if (allProducts.empty())
{
            cout<<"No products to display."<<endl;
}
        else
{
            for (int i=0; i<allProducts.size(); i++)
{
                cout<<(i+1)<<". ";
                allProducts[i].display();
}
}
        cout<<"Press Enter to continue...";
        safeCinClear();
        cin.get();
}

    void addToCart()
{
        clearScreen();
        int productNumber;
        int quantityToBuy;
        cout<<BOLD_TEXT<<BLUE_COLOR<<"::::: Add to Cart :::::"<<RESET_COLOR<<endl;

        if (allProducts.empty())
{
            cout<<"No products available to add to cart."<<endl;
            cout<<"Press Enter to continue...";
            safeCinClear();
            cin.get();
            return;
}

        cout<<BOLD_TEXT<<CYAN_COLOR<<"::::: Available Products :::::"<<RESET_COLOR<<endl;
        for (int i=0; i < allProducts.size(); i++)
{
            cout<<(i + 1)<<". ";
            allProducts[i].display();
}

        cout<<"\nEnter the number of the product to add to cart: ";
        while (!(cin >> productNumber) || productNumber<=0 || productNumber>(allProducts.size()))
{
            cout<<RED_COLOR<<"Invalid product number. Please enter a valid number: "<<RESET_COLOR;
            safeCinClear();
}

        Product& selectedProduct = allProducts[productNumber - 1];

        cout<<"Enter quantity to buy for "<<selectedProduct.getName()<<": ";
        while (!(cin>>quantityToBuy) || quantityToBuy<=0)
{
            cout<<RED_COLOR<<"Invalid quantity. Please enter a positive number: "<<RESET_COLOR;
            safeCinClear();
}

        if (selectedProduct.getQuantity()>=quantityToBuy)
{
            bool inCart=false;
            for (int i=0; i<cart.size(); i++)
{
                if (cart[i].getName()==selectedProduct.getName())
{
                    cart[i].setQuantity(cart[i].getQuantity() + quantityToBuy);
                    inCart=true;
                    break;
}
}
            if (!inCart)
{
                cart.push_back(Product(selectedProduct.getName(), selectedProduct.getPrice(), quantityToBuy,
                                       selectedProduct.getSellerUsername(), selectedProduct.getCategory()));
}
            selectedProduct.setQuantity(selectedProduct.getQuantity() - quantityToBuy);
            cout<<GREEN_COLOR<<"Added "<<quantityToBuy<<" of "<<selectedProduct.getName()<<" to cart!"<<RESET_COLOR<<endl;
            saveAllData();
}
        else
{
            cout<<RED_COLOR<<"Not enough stock for "<<selectedProduct.getName()<<". Available: "<<selectedProduct.getQuantity()<<RESET_COLOR<<endl;
}
        cout<<"Press Enter to continue...";
        safeCinClear();
        cin.get();
}

    void deleteFromCart()
{
        clearScreen();
        cout<<BOLD_TEXT<<BLUE_COLOR<<"::::: Remove from Cart :::::"<<RESET_COLOR<<endl;

        if (cart.empty())
{
            cout<<"Your cart is empty."<<endl;
            cout<<"Press Enter to continue...";
            safeCinClear();
            cin.get();
            return;
}

        cout<<BOLD_TEXT<<CYAN_COLOR<<"::::: Your Cart :::::"<<RESET_COLOR<<endl;
        for (int i=0; i<cart.size(); i++)
{
            cout<<(i+1)<<". ";
            cart[i].display();
}

        cout<<"\nEnter the number of the product to remove from cart: ";
        int productNumber;
        while (!(cin >> productNumber) || productNumber<=0 || productNumber>(cart.size()))
{
            cout<<RED_COLOR<<"Invalid product number. Please enter a valid number: "<<RESET_COLOR;
            safeCinClear();
}

        Product& selectedCartItem = cart[productNumber - 1];
        int quantityInCart = selectedCartItem.getQuantity();

        for (int i=0; i<allProducts.size(); i++)
{
            if (allProducts[i].getName() == selectedCartItem.getName() &&
                    allProducts[i].getSellerUsername() == selectedCartItem.getSellerUsername())
{
                allProducts[i].setQuantity(allProducts[i].getQuantity() + quantityInCart);
                break;
}
}

        cart.erase(cart.begin() + (productNumber - 1));
        cout<<GREEN_COLOR<<"Removed "<<selectedCartItem.getName()<<" from cart!"<<RESET_COLOR<<endl;
        saveAllData();
        cout<<"Press Enter to continue...";
        safeCinClear();
        cin.get();
}

    void editCartItem()
{
        clearScreen();
        cout<<BOLD_TEXT<<BLUE_COLOR<<"::::: Edit Cart Item :::::"<<RESET_COLOR<<endl;

        if (cart.empty())
{
            cout<<"Your cart is empty."<<endl;
            cout<<"Press Enter to continue...";
            safeCinClear();
            cin.get();
            return;
}

        cout<<BOLD_TEXT<<CYAN_COLOR<<"::::: Your Cart :::::"<<RESET_COLOR<<endl;
        for (int i=0; i<cart.size(); i++)
{
            cout<<(i+1)<<". ";
            cart[i].display();
}

        cout<<"\nEnter the number of the product to edit: ";
        int productNumber;
        while (!(cin >> productNumber) || productNumber<=0 || productNumber>(cart.size()))
{
            cout<<RED_COLOR<<"Invalid product number. Please enter a valid number: "<<RESET_COLOR;
            safeCinClear();
}

        Product& selectedCartItem = cart[productNumber - 1];
        int currentQuantity = selectedCartItem.getQuantity();
        int availableStock = 0;

        for (int i=0; i<allProducts.size(); i++)
{
            if (allProducts[i].getName() == selectedCartItem.getName() &&
                    allProducts[i].getSellerUsername() == selectedCartItem.getSellerUsername())
{
                availableStock = allProducts[i].getQuantity();
                break;
}
}

        cout<<"Current quantity of "<<selectedCartItem.getName()<<": "<<currentQuantity<<endl;
        cout<<"Available stock in inventory: "<<availableStock<<endl;
        cout<<"Enter new quantity (0 to remove item): ";
        int newQuantity;
        while (!(cin>>newQuantity) || newQuantity<0)
{
            if (cin.eof())
{
                cout<<RED_COLOR<<"EOF detected...Returning to main menu..."<<RESET_COLOR<<endl;
                return;
}
            cout<<RED_COLOR<<"Invalid quantity. Please enter a non-negative number: "<<RESET_COLOR;
            safeCinClear();
}

        if (newQuantity == 0)
{

            for (int i=0; i<allProducts.size(); i++)
{
                if (allProducts[i].getName() == selectedCartItem.getName() &&
                        allProducts[i].getSellerUsername() == selectedCartItem.getSellerUsername())
{
                    allProducts[i].setQuantity(allProducts[i].getQuantity() + currentQuantity);
                    break;
}
}
            cart.erase(cart.begin() + (productNumber - 1));
            cout<<GREEN_COLOR<<"Removed "<<selectedCartItem.getName()<<" from cart!"<<RESET_COLOR<<endl;
}
        else if (newQuantity <= (currentQuantity + availableStock))
{

            int quantityDiff = newQuantity - currentQuantity;
            for (int i=0; i<allProducts.size(); i++)
{
                if (allProducts[i].getName() == selectedCartItem.getName() &&
                        allProducts[i].getSellerUsername() == selectedCartItem.getSellerUsername())
{
                    allProducts[i].setQuantity(allProducts[i].getQuantity() - quantityDiff);
                    break;
}
}
            selectedCartItem.setQuantity(newQuantity);
            cout<<GREEN_COLOR<<"Updated quantity of "<<selectedCartItem.getName()<<" to "<<newQuantity<<"!"<<RESET_COLOR<<endl;
}
        else
{
            cout<<RED_COLOR<<"Not enough stock for "<<selectedCartItem.getName()<<". Available: "<<availableStock<<RESET_COLOR<<endl;
}

        saveAllData();
        cout<<"Press Enter to continue...";
        safeCinClear();
        cin.get();
}

    void viewCart() const
{
        clearScreen();
        cout<<BOLD_TEXT<<YELLOW_COLOR<<"::::: Your Cart :::::"<<RESET_COLOR<<endl;
        if (cart.empty())
{
            cout<<"Your cart is empty."<<endl;
}
        else
{
            for (int i=0; i<cart.size(); i++)
{
                cout<<(i+1)<<". ";
                cart[i].display();
}
}
        cout<<"Press Enter to continue...";
        safeCinClear();
        cin.get();
}

    void checkout()
{
        clearScreen();
        cout<<BOLD_TEXT<<GREEN_COLOR<<"::::: Checkout :::::"<<RESET_COLOR<<endl;
        if (cart.empty())
{
            cout<<"Your cart is empty. Nothing to checkout."<<endl;
            cout<<"Press Enter to continue...";
            safeCinClear();
            cin.get();
            return;
}

        double totalCost =0.0;
        for (int i=0; i<cart.size(); i++)
{
            totalCost+=cart[i].getPrice()*cart[i].getQuantity();
}


        cout<<"Total cost of your cart: $"<<totalCost<<endl;
        cout<<"Your current balance: $"<<balance<<endl;

        if (balance>=totalCost)
{
            cout<<"Processing payment..."<<endl;
            withdraw(totalCost);

            for (int i=0; i<cart.size(); i++)
{
                for (int j=0; j<allUsers.size(); j++)
{
                    if (allUsers[j]->getType()=="Seller" && allUsers[j]->getUsername()==cart[i].getSellerUsername())
{
                        allUsers[j]->deposit(cart[i].getPrice() * cart[i].getQuantity());

                        cout<<GREEN_COLOR<<"Transferred $"<<cart[i].getPrice() * cart[i].getQuantity()<<" to seller "<<cart[i].getSellerUsername()<<RESET_COLOR<<endl;
                        break;
}
}
}
            cart.clear();
            cout<<GREEN_COLOR<<"Payment successful! Your new balance: $"<<balance<<RESET_COLOR<<endl;
            saveAllData();
}

        else
{
            cout<<RED_COLOR<<"Insufficient funds. Please add money to your account."<<RESET_COLOR<<endl;
}

        cout<<"Press Enter to continue...";
        safeCinClear();
        cin.get();
}

    void menu()
{
        int choice;
        do
{
            clearScreen();
            stringstream balanceStream;
            balanceStream<<fixed<<setprecision(2)<<balance;
            cout<<BOLD_TEXT<<MAGENTA_COLOR<<"--- Buyer Menu ---"<<RESET_COLOR<<endl;
            cout<<BOLD_TEXT<<"Welcome, "<<username<<"! Your Balance: $"<<balanceStream.str()<<RESET_COLOR<<endl;
            cout<<"1. Browse Products\n";
            cout<<"2. Add to Cart\n";
            cout<<"3. View Cart\n";
            cout<<"4. Edit Cart Item\n";
            cout<<"5. Checkout\n";
            cout<<"6. Search Products\n";
            cout<<"7. Sort Products by Price\n";
            cout<<"8. Update Balance\n";
            cout<<"0. Logout\n";
            cout<<BOLD_TEXT<<YELLOW_COLOR<<"Choice: "<<RESET_COLOR;

            while (!(cin>>choice))
{
                cout<<RED_COLOR<<"Invalid input. Please enter a number: "<<RESET_COLOR;
                safeCinClear();
}

            switch (choice)
{
            case 1:
                browseProductsMenu();
                break;
            case 2:
                addToCart();
                break;
            case 3:
                viewCart();
                break;
            case 4:
                editCartItem();
                break;
            case 5:
                checkout();
                break;
            case 6:
                searchProducts();
                break;
            case 7:
                sortProducts();
                break;
            case 8:
                updateBalance();
                break;
            case 0:
                cout<<"Logging out...\n";
                break;
            default:
                cout<<RED_COLOR<<"Invalid choice"<<RESET_COLOR<<endl;
                cout<<"Press Enter to continue...";
                safeCinClear();
                cin.get();
}
}
        while (choice != 0);
}
};

bool isValidUsername(const string& username)
{
    bool hasAlpha=false, hasDigit=false;
    if (username.length() < 3)
        return false;
    for (int i = 0; i < username.length(); i++)
{
        char ch=username[i];
        if (isalpha(ch))
            hasAlpha=true;
        else if (isdigit(ch))
            hasDigit=true;
        else if (ch == '_' || ch == '-')
            continue;
        else
            return false;
}
    return hasAlpha && hasDigit;
}

bool isValidPassword(const string& password)
{
    return password.length()>=8;
}

bool usernameExists(const string& uname)
{
    for (int i=0; i<allUsers.size(); i++)
{
        if (allUsers[i]->getUsername()==uname)
            return true;
}
    return false;
}

User* signup(int type)
{
    clearScreen();
    string username, password;
    cout<<BOLD_TEXT<<GREEN_COLOR<<"::::: User Sign Up :::::"<<RESET_COLOR<<endl;

    while (true)
{
        cout<<"Enter username (min 3 chars, must contain digits and alphabets, can use _-): ";
        cin>>username;
        if (!isValidUsername(username))
{
            cout<<RED_COLOR<<"Invalid username format. Try again."<<RESET_COLOR<<endl;
            safeCinClear();
            continue;
}
        if (usernameExists(username))
{
            cout<<RED_COLOR<<"Username already exists. Try another one."<<RESET_COLOR<<endl;
            safeCinClear();
            continue;
}
        break;
}

    while (true)
{
        cout<<"Enter password (at least 8 characters): ";
        cin>>password;
        if (!isValidPassword(password))
{
            cout<<RED_COLOR<<"Password must be at least 8 characters."<<RESET_COLOR<<endl;
            safeCinClear();
            continue;
}
        break;
}

    User* newUser=nullptr;
    if (type==1)
{
        newUser=new Seller(username, password,0.0);
}
    else
{
        newUser=new Buyer(username, password, 1000.00);
}
    allUsers.push_back(newUser);
    saveAllData();
    cout<<GREEN_COLOR<<"Signup successful. Please login!"<<RESET_COLOR<<endl;
    cout<<"Press Enter to continue...";
    safeCinClear();
    cin.get();
    return newUser;
}

User* login(int type)
{
    clearScreen();
    string username, password;
    cout<<BOLD_TEXT<<BLUE_COLOR<<"::::: User Login :::::"<<RESET_COLOR<<endl;
    cout<<"Enter username: ";
    cin>>username;
    cout<<"Enter password: ";
    cin>>password;

    for (int i=0; i<allUsers.size(); i++)
{
        if (allUsers[i]->getUsername()==username && allUsers[i]->getPassword()==password)
{
            if ((type==1 && allUsers[i]->getType()=="Seller") ||(type==2 && allUsers[i]->getType()=="Buyer"))
{
                cout<<GREEN_COLOR<<"Login successful!"<<RESET_COLOR<<endl;
                cout<<"Press Enter to continue...";
                safeCinClear();
                cin.get();
                return allUsers[i];
}
}
}
    cout<<RED_COLOR<<"Incorrect username or password, or incorrect role."<<RESET_COLOR<<endl;
    cout<<"Press Enter to continue...";
    safeCinClear();
    cin.get();
    return nullptr;
}

void clearScreen()
{
    system("cls");
}

void safeCinClear()
{
    cin.clear();
    cin.ignore(10000, '\n');
}

void saveAllData()
{
    ofstream productsFile("products.txt");
    if (!productsFile.is_open())
{
        cout<<RED_COLOR<<"ERROR unable to open file"<<RESET_COLOR<<endl;
        return;
}
    for (int i=0; i<allProducts.size(); i++)
{
        string productStr=allProducts[i].toString();
        if (!productStr.empty())
            productsFile<<productStr<<endl;
}
    productsFile.close();

    ofstream usersFile("users.txt");
    if (!usersFile.is_open())
{
        cout<<RED_COLOR<<"ERROR unable to open file"<<RESET_COLOR<<endl;
        return;
}
    for (int i=0; i<allUsers.size(); i++)
{
        usersFile<<allUsers[i]->toString()<<endl;
}
    usersFile.close();
}

void loadAllData()
{
    allProducts.clear();
    for (int i=0; i<allUsers.size(); i++)
{
        delete allUsers[i];
}
    allUsers.clear();

    ifstream productsFile("products.txt");
    if (productsFile.is_open())
{
        string line;
        while (getline(productsFile, line))
{
            if (line.empty())
                continue;
            bool success=false;
            Product product=Product::fromString(line, success);
            if (success)
                allProducts.push_back(product);
}
        productsFile.close();
}

    else

{
        cout<<YELLOW_COLOR<<"products.txt not found  Starting with empty product list."<<RESET_COLOR<<endl;
}

    ifstream usersFile("users.txt");
    if (usersFile.is_open())
{
        string line;
        while (getline(usersFile, line))
{
            if (line.empty())
                continue;
            stringstream ss(line);
            string username, password,balanceStr,type,cartData;

            if (!getline(ss, username, ',') || !getline(ss, password, ',') ||
                    !getline(ss, balanceStr, ',') || !getline(ss, type, ','))
{
                cout<<RED_COLOR<<"Error: Invalid user format in line: '"<<line<<"'"<<RESET_COLOR<<endl;
                continue;
}

            getline(ss, cartData);
            if (!cartData.empty() && cartData[0] == ',')
                cartData = cartData.substr(1);

            try
{
                stringstream balanceSS(balanceStr);
                double balance;
                balanceSS>>balance;
                if (balanceSS.fail())
                    throw "Invalid balance";

                User* user=nullptr;
                if (type=="Seller")
{
                    user=new Seller(username,password,balance);
}
                else if (type == "Buyer")
{
                    Buyer* buyer=new Buyer(username,password,balance);
                    if (!cartData.empty())
{
                        buyer->loadCartFromString(cartData);
}
                    user=buyer;
}

                if (user)
                    allUsers.push_back(user);
                else
                    cout<<RED_COLOR<<"Error: Unknown user type in line: '"<<line<<"'"<<RESET_COLOR<<endl;
}
            catch (...)
{
                cout<<RED_COLOR<<"Error: Invalid balance in line: '"<<line<<"'"<<RESET_COLOR<<endl;
}
}
        usersFile.close();
}
    else
{
        cout<<YELLOW_COLOR<<"users.txt not found Starting with empty user list"<<RESET_COLOR<<endl;
}
}

int main()
{
    loadAllData();

    CURLcode globalInitResult=curl_global_init(CURL_GLOBAL_ALL);
    if (globalInitResult!=CURLE_OK)
{
        cerr<<RED_COLOR<<"libcurl global initialization failed "<<curl_easy_strerror(globalInitResult)<<RESET_COLOR<<endl;
        return 1;
}

    int choice;
    do
{
        clearScreen();
       cout<<"\t\t\t";
        cout<<BOLD_TEXT<<CYAN_COLOR<<"\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2"<<endl;
        cout<<"\t\t\t";
        cout<<"\1SMART SHOPPING SYSTEM \1"<<endl;
        cout<<"\t\t\t";
        cout<<"\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2\2"<<endl<<RESET_COLOR;
        cout<<BLUE_COLOR<<"\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\t\t\t\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4"<<endl;
        cout<<"\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\t\t\t\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4"<<endl;cout<<"\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\t\t\t\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4\4"<<RESET_COLOR<<endl<<endl;
        cout<<BOLD_TEXT<<"1. Sign Up\n";
        cout<<"2. Login\n";
        cout<<GREEN_COLOR<<"3. Customer Support (Chatbot)\n"<<RESET_COLOR;
        cout<<RED_COLOR<<"0. Exit\n"<<RESET_COLOR;
        cout<<BOLD_TEXT<<YELLOW_COLOR<<"Choice: "<<RESET_COLOR;

        while (!(cin>>choice))
{
            cout<<RED_COLOR<<"Invalid input. Please enter a number: "<<RESET_COLOR;
            safeCinClear();
}

        if (choice==0)
{
            cout<<GREEN_COLOR<<"Exiting Smart Shopping System. Goodbye!\n";
            break;
}
        else if (choice==3)
{
            runChatbot();
            continue;
}

        int type;
        cout<<"Select role:\n";
        cout<<"1. Seller\n";
        cout<<"2. Buyer\n";
        cout<<BOLD_TEXT<<YELLOW_COLOR<<"Role Choice: "<<RESET_COLOR;
        while (!(cin>>type) || (type!=1 && type!=2))
{
            cout<<RED_COLOR<<"Invalid role choice. Please enter 1 for Seller or 2 for Buyer: "<<RESET_COLOR;
            safeCinClear();
}

        User* currentUser=nullptr;

        if (choice==1)
{
            currentUser=signup(type);
}
        else if (choice==2)
{
            currentUser=login(type);
            if (currentUser!=nullptr)
{
                currentUser->menu();
}
}
        else
{
            cout<<RED_COLOR<<"Invalid choice. Please try again."<<RESET_COLOR<<endl;
            cout<<"Press Enter to continue...";
            safeCinClear();
            cin.get();
}
}
    while (true);

    for (int i=0; i<allUsers.size(); i++)
{
        delete allUsers[i];
}
    allUsers.clear();

    curl_global_cleanup();
    return 0;
}
