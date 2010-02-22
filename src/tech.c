/*
 * See Licensing and Copyright notice in naev.h
 */

/**
 * @file tech.c
 *
 * @brief Handles tech groups and metagroups for populating the planet outfitter,
 *        shipyard and commodity exchange.
 */


#include "tech.h"

#include "naev.h"

#include "nxml.h"
#include "log.h"
#include "ndata.h"
#include "outfit.h"
#include "ship.h"
#include "economy.h"
#include "array.h"


#define TECH_DATA           "dat/tech.xml"   /**< XML file containing techs. */
#define XML_TECH_ID         "Techs"          /**< Tech xml document tag. */
#define XML_TECH_TAG        "tech"           /**< Individual tech xml tag. */


/**
 * @brief Different tech types.
 */
typedef enum tech_item_type_e {
   TECH_TYPE_OUTFIT,       /**< Tech contains an outfit. */
   TECH_TYPE_SHIP,         /**< Tech contains a ship. */
   TECH_TYPE_COMMODITY,    /**< Tech contains a commodity. */
   TECH_TYPE_CONTRABAND,   /**< Tech contains contraband. */
   TECH_TYPE_GROUP,        /**< Tech contains another tech group. */
} tech_item_type_t;


/**
 * @brief Item contained in a tech group.
 */
typedef struct tech_item_s {
   tech_item_type_t type;  /**< Type of data. */
   union {
      Outfit *outfit;      /**< Outfit pointer. */
      Ship *ship;          /**< Ship pointer. */
      Commodity *comm;     /**< Commodity pointer. */
      int grp;             /**< Identifier of another tech group. */
   } u;                    /**< Data union. */
} tech_item_t;


/**
 * @brief Group of tech items, basic unit of the tech trees.
 */
typedef struct tech_group_s {
   char *name;          /**< Name of the tech group. */
   tech_item_t *items;  /**< Items in the tech group. */
} tech_group_t;


/*
 * Group list.
 */
static tech_group_t *tech_groups = NULL;


/*
 * Prototypes.
 */
static tech_item_t *tech_itemGrow( tech_group_t *grp );
static int tech_parseNode( tech_group_t *tech, xmlNodePtr parent );
static int tech_parseNodeData( tech_group_t *tech, xmlNodePtr parent );
static int tech_loadItemOutfit( tech_group_t *grp, xmlNodePtr parent );
static int tech_loadItemShip( tech_group_t *grp, xmlNodePtr parent );
static int tech_loadItemCommodity( tech_group_t *grp, xmlNodePtr parent );
static int tech_loadItemGroup( tech_group_t *grp, xmlNodePtr parent );


/**
 * @brief Loads the tech information.
 */
int tech_load (void)
{
   int i, ret, s;
   uint32_t bufsize;
   char *buf;
   xmlNodePtr node;
   xmlDocPtr doc;
   tech_group_t *tech;

   /* Create thea rray. */
   tech_groups = array_create( tech_group_t );

   /* Load the data. */
   buf = ndata_read( TECH_DATA, &bufsize );
   if (buf == NULL)
      return -1;

   /* Load the document. */
   doc = xmlParseMemory( buf, bufsize );
   if (doc == NULL) {
      WARN("'%s' is not a valid XML file.", TECH_DATA);
      return -1;
   }

   /* Load root element. */
   node = doc->xmlChildrenNode;
   if (!xml_isNode(node,XML_TECH_ID)) {
      WARN("Malformed "TECH_DATA" file: missing root element '"XML_TECH_ID"'");
      return -1;
   }

   /* Get first node. */
   node = node->xmlChildrenNode; /* first system node */
   if (node == NULL) {
      WARN("Malformed "TECH_DATA" file: does not contain elements");
      return -1;
   }

   /* First pass create the groups - needed to reference them later. */
   ret = 0;
   do {
      if (ret == 0) /* Write over failures. */
         tech = &array_grow( &tech_groups );
      ret = tech_parseNode( tech, node );
   } while (xml_nextNode(node));
   array_shrink( &tech_groups );

   /* Now we load the data. */
   node  = node->xmlChildrenNode; /* first system node */
   i     = 0;
   s     = array_size( &tech_groups );
   do {
      /* Must aviod warning by checking explicit NULL. */
      buf   = xml_get(node);
      if (buf == NULL)
         continue;

      /* Load next tech. */
      for (  ; i<s; i++) {
         tech  = &tech_groups[i];
         if (strcmp(tech->name, buf)==0)
            tech_parseNodeData( tech, node );
      }
   } while (xml_nextNode(node));

   return 0;
}


/**
 * @brief Cleans up after the tech stuff.
 */
void tech_free (void)
{
   int i, s;

   /* Free all individual techs. */
   s = array_size( tech_groups );
   for (i=0; i<s; i++) {
   }

   /* Free the tech array. */
   array_free( tech_groups );
}


/**
 * @brief Parses an XML tech node.
 */
static int tech_parseNode( tech_group_t *tech, xmlNodePtr parent )
{
   /* Get name. */
   xmlr_attr( parent, "name", tech->name);
   if (tech->name == NULL) {
      WARN("tech node does not have 'name' attribute");
      return 1;
   }

   return 0;
}


/**
 * @brief Parses an XML tech node.
 */
static int tech_parseNodeData( tech_group_t *tech, xmlNodePtr parent )
{
   xmlNodePtr node;
   char *buf;
   int ret;

   /* Parse the data. */
   node = parent->xmlChildrenNode;
   do {
      if (xml_isNode(node,"item")) {
         /* Try to find hardcoded type. */
         buf = xml_nodeProp( node, "type" );
         if (buf == NULL) {
            ret = 0;
            if (!ret)
               ret = tech_loadItemGroup( tech, node );
            if (!ret)
               ret = tech_loadItemOutfit( tech, node );
            if (!ret)
               ret = tech_loadItemShip( tech, node );
            if (!ret)
               ret = tech_loadItemCommodity( tech, node );
            if (!ret) {
               WARN("Generic item '%s' not found in tech group '%s'",
                     xml_get(node), tech->name );
               continue;
            }
         }
         else if (strcmp(buf,"group")==0) {
            if (!tech_loadItemGroup( tech, node )) {
               WARN("Group item '%s' not found in tech group '%s'.",
                     xml_get(node), tech->name );
               continue;
            }
         }
         else if (strcmp(buf,"outfit")==0) {
            if (!tech_loadItemGroup( tech, node )) {
               WARN("Outfit item '%s' not found in tech group '%s'.",
                     xml_get(node), tech->name );
               continue;
            }
         }
         else if (strcmp(buf,"ship")==0) {
            if (!tech_loadItemGroup( tech, node )) {
               WARN("Ship item '%s' not found in tech group '%s'.",
                     xml_get(node), tech->name );
               continue;
            }
         }
         else if (strcmp(buf,"commodity")==0) {
            if (!tech_loadItemGroup( tech, node )) {
               WARN("Commodity item '%s' not found in tech group '%s'.",
                     xml_get(node), tech->name );
               continue;
            }
         }
      }
   } while (xml_nextNode( node ));

   return 0;
}


/**
 * @brief Adds an item to a tech.
 */
static tech_item_t *tech_itemGrow( tech_group_t *grp )
{
   if (grp->items == NULL)
      grp->items = array_create( tech_item_t );
   return &array_grow( &grp->items );
}


/**
 * @brief Loads a group item pertaining to a outfit.
 *
 *    @return 0 on success.
 */
static int tech_loadItemOutfit( tech_group_t *grp, xmlNodePtr parent )
{
   char *name;
   tech_item_t *item;
   Outfit *o;

   /* Get the name. */
   name = xml_get( parent );
   if (name == NULL) {
      WARN("Tech group '%s' has an item without a value.", grp->name);
      return -1;
   }

   /* Get the outfit. */
   o = outfit_getW( name );
   if (o==NULL)
      return 1;
 
   /* Load the new item. */
   item           = tech_itemGrow( grp );
   item->type     = TECH_TYPE_OUTFIT;
   item->u.outfit = o;
   return 0;
}


/**
 * @brief Loads a group item pertaining to a outfit.
 *
 *    @return 0 on success.
 */
static int tech_loadItemShip( tech_group_t *grp, xmlNodePtr parent )
{
   char *name;
   tech_item_t *item;
   Ship *s;

   /* Get the name. */
   name = xml_get( parent );
   if (name == NULL) {
      WARN("Tech group '%s' has an item without a value.", grp->name);
      return -1;
   }

   /* Get the outfit. */
   s = ship_getW( name );
   if (s==NULL)
      return 1;
 
   /* Load the new item. */
   item           = tech_itemGrow( grp );
   item->type     = TECH_TYPE_SHIP;
   item->u.ship   = s;
   return 0;
}


/**
 * @brief Loads a group item pertaining to a outfit.
 *
 *    @return 0 on success.
 */
static int tech_loadItemCommodity( tech_group_t *grp, xmlNodePtr parent )
{
   char *name;
   tech_item_t *item;
   Commodity *c;

   /* Get the name. */
   name = xml_get( parent );
   if (name == NULL) {
      WARN("Tech group '%s' has an item without a value.", grp->name);
      return -1;
   }

   /* Get the outfit. */
   c = commodity_getW( name );
   if (c==NULL)
      return 1;
 
   /* Load the new item. */
   item           = tech_itemGrow( grp );
   item->type     = TECH_TYPE_COMMODITY;
   item->u.comm   = c;
   return 0;
}


/**
 * @brief Loads a group item pertaining to a group.
 *
 *    @return 0 on success.
 */
static int tech_loadItemGroup( tech_group_t *grp, xmlNodePtr parent )
{
   char *name;
   tech_item_t *item;

   /* Get the name. */
   name = xml_get( parent );
   if (name == NULL) {
      WARN("Tech group '%s' has an item without a value.", grp->name);
      return -1;
   }

   /* Load the new item. */
   item           = tech_itemGrow( grp );
   item->type  = TECH_TYPE_GROUP;
   return 0;
}



