
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __MLMenu__
#define __MLMenu__

#include "JuceHeader.h"
#include "MLLookAndFeel.h"
#include "MLSymbol.h"
#include "MLTextUtils.h"

class MLMenu;

// TODO use ml::ResourceMap. Possibly just create the menu tree from an external map every time we show the menu. 
// or, add method to set contents from a tree / resourceMap value. 
// need to be able to add arbitrary submaps .
// TODO add forward / back button option and persist a current selection. 
// Use index or store using new bidirectional ml::ResourceMap iterator .

typedef std::shared_ptr<MLMenu> MLMenuPtr;
typedef std::map<ml::Symbol, MLMenuPtr> MLMenuMapT;
typedef std::shared_ptr<juce::PopupMenu> JuceMenuPtr;

// adapter to Juce menu
class MLMenu
{
public:
    class Node;
    typedef std::shared_ptr<Node> NodePtr;
	
	// MLTEST rewrite with textUtils compare on Symbols
	struct collator 
	{
		bool operator()(const std::string& a, const std::string& b) const 
		{
			return ml::textUtils::collate(ml::TextFragment(a.c_str()), ml::TextFragment(b.c_str()));
		}
	};
	
	typedef std::map< std::string, NodePtr, collator > StringToMenuNodeMapT;
  
	class Node
    {
    public:
        Node();
        ~Node();
        void clear();
        bool isValid();
        void dump(int level = 0);
        int renumberItems(int n = 1);
        int getNodeSize(int n);
        void buildFullNameIndex(std::vector<std::string>& nameVec, const std::string& path);
        void addToJuceMenu(const std::string& name, JuceMenuPtr pMenu, bool root = true);
		void setDisplayPrefix(std::string p) { mDisplayPrefix = ml::TextFragment(p.c_str()); }
		const std::list<std::string>& getIndex() { return index; }
		NodePtr getSubnodeByName(const std::string& name);
		
		// TODO use ml::Symbols as map keys instead. 
        StringToMenuNodeMapT map;
        std::list<std::string> index;
		ml::TextFragment mDisplayPrefix;
		JuceMenuPtr subMenu;
        int mItemNumber;
        bool mEnabled;
		bool mTicked;
    };

	MLMenu();
	MLMenu(const ml::Symbol name);
	~MLMenu();
    
	void clear();
	void addItem(const std::string& name, bool enabled = true, bool ticked = false);
	void addSeparator();
	NodePtr getItem(const std::string& name);
	void addItems(const std::vector<std::string>& items);
	void addSubMenu(MLMenuPtr m);
	void addSubMenu(MLMenuPtr m, const std::string& name);
	void appendMenu(MLMenuPtr m);
    void buildIndex();
    
	ml::Symbol getName() { return mName; }
	int getSize() { return mRoot->getNodeSize(0); }
	
	// TODO return TextFragment
	const std::string getMenuItemPath(int idx);
    
    // build a Juce menu on the fly and return it
	JuceMenuPtr getJuceMenu();	

	void setInstigator(ml::Symbol n) {mInstigatorName = n;}
	ml::Symbol getInstigator() const {return mInstigatorName;}
    
    void dump();

private:	
	ml::Symbol mName;
	ml::Symbol mInstigatorName; // name of Widget that triggered us
    NodePtr mRoot;
    bool mHasIndex;
    std::vector<std::string> mFullNamesByIndex;
};


#endif // __MLMenu__
