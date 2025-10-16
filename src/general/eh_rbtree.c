#include <stddef.h>
#include <stdbool.h>
#include <eh_rbtree.h>

static inline void rb_set_parent(struct eh_rbtree_node *rb, struct eh_rbtree_node *p)
{
    rb->parent_and_color = rb_color(rb) + (parent_node_t)p;
}

static inline void rb_set_parent_color(struct eh_rbtree_node *rb,
                       struct eh_rbtree_node *p, int color)
{
    rb->parent_and_color = (parent_node_t)p + (parent_node_t)color;
}

static inline void __rb_change_child(struct eh_rbtree_node *old, struct eh_rbtree_node *new,
          struct eh_rbtree_node *parent, struct eh_rbtree_root *root)
{
    if (parent) {
        if (parent->rb_left == old)
            parent->rb_left = new;
        else
            parent->rb_right = new;
    } else
        root->rb_node = new;
}

static struct eh_rbtree_node *rb_left_deepest_node(const struct eh_rbtree_node *node)
{
    for (;;) {
        if (node->rb_left)
            node = node->rb_left;
        else if (node->rb_right)
            node = node->rb_right;
        else
            return (struct eh_rbtree_node *)node;
    }
}

static inline void rb_set_black(struct eh_rbtree_node *rb)
{
    rb->parent_and_color |= RB_BLACK;
}

static inline struct eh_rbtree_node *rb_red_parent(struct eh_rbtree_node *red)
{
    return (struct eh_rbtree_node *)red->parent_and_color;
}

static inline void rb_link_node(struct eh_rbtree_node *node, struct eh_rbtree_node *parent,
                struct eh_rbtree_node **rb_link)
{
    node->parent_and_color = (parent_node_t)parent;
    node->rb_left = node->rb_right = NULL;

    *rb_link = node;
}


/*
 * Helper function for rotations:
 * - old's parent and color get assigned to new
 * - old gets assigned new as a parent and 'color' as a color.
 */
static inline void
__rb_rotate_set_parents(struct eh_rbtree_node *old, struct eh_rbtree_node *new,
            struct eh_rbtree_root *root, int color)
{
    struct eh_rbtree_node *parent = rb_parent(old);
    new->parent_and_color = old->parent_and_color;
    rb_set_parent_color(old, new, color);
    __rb_change_child(old, new, parent, root);
}

static void
__rb_insert(struct eh_rbtree_node *node, struct eh_rbtree_root *root)
{
    struct eh_rbtree_node *parent = rb_red_parent(node), *gparent, *tmp;

    while (true) {
        /*
         * Loop invariant: node is red.
         */
        if (eh_unlikely(!parent)) {
            /*
             * The inserted node is root. Either this is the
             * first node, or we recursed at Case 1 below and
             * are no longer violating 4).
             */
            rb_set_parent_color(node, NULL, RB_BLACK);
            break;
        }

        /*
         * If there is a black parent, we are done.
         * Otherwise, take some corrective action as,
         * per 4), we don't want a red root or two
         * consecutive red nodes.
         */
        if(rb_is_black(parent))
            break;

        gparent = rb_red_parent(parent);

        tmp = gparent->rb_right;
        if (parent != tmp) {    /* parent == gparent->rb_left */
            if (tmp && rb_is_red(tmp)) {
                /*
                 * Case 1 - node's uncle is red (color flips).
                 *
                 *       G            g
                 *      / \          / \
                 *     p   u  -->   P   U
                 *    /            /
                 *   n            n
                 *
                 * However, since g's parent might be red, and
                 * 4) does not allow this, we need to recurse
                 * at g.
                 */
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->rb_right;
            if (node == tmp) {
                /*
                 * Case 2 - node's uncle is black and node is
                 * the parent's right child (left rotate at parent).
                 *
                 *      G             G
                 *     / \           / \
                 *    p   U  -->    n   U
                 *     \           /
                 *      n         p
                 *
                 * This still leaves us in violation of 4), the
                 * continuation into Case 3 will fix that.
                 */
                tmp = node->rb_left;
                parent->rb_right = tmp;
                node->rb_left = parent;
                if (tmp)
                    rb_set_parent_color(tmp, parent,
                                RB_BLACK);
                rb_set_parent_color(parent, node, RB_RED);
                parent = node;
                tmp = node->rb_right;
            }

            /*
             * Case 3 - node's uncle is black and node is
             * the parent's left child (right rotate at gparent).
             *
             *        G           P
             *       / \         / \
             *      p   U  -->  n   g
             *     /                 \
             *    n                   U
             */
            gparent->rb_left = tmp;
            parent->rb_right = gparent;
            if (tmp)
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            __rb_rotate_set_parents(gparent, parent, root, RB_RED);
            break;
        } else {
            tmp = gparent->rb_left;
            if (tmp && rb_is_red(tmp)) {
                /* Case 1 - color flips */
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->rb_left;
            if (node == tmp) {
                /* Case 2 - right rotate at parent */
                tmp = node->rb_right;
                parent->rb_left = tmp;
                node->rb_right = parent;
                if (tmp)
                    rb_set_parent_color(tmp, parent,
                                RB_BLACK);
                rb_set_parent_color(parent, node, RB_RED);
                parent = node;
                tmp = node->rb_left;
            }

            /* Case 3 - left rotate at gparent */
            gparent->rb_right = tmp;
            parent->rb_left = gparent;
            if (tmp)
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            __rb_rotate_set_parents(gparent, parent, root, RB_RED);
            break;
        }
    }
}

/*
 * Inline version for rb_erase() use - we want to be able to inline
 * and eliminate the dummy_rotate callback there
 */
static void
____rb_erase_color(struct eh_rbtree_node *parent, struct eh_rbtree_root *root)
{
    struct eh_rbtree_node *node = NULL, *sibling, *tmp1, *tmp2;

    while (true) {
        /*
         * Loop invariants:
         * - node is black (or NULL on first iteration)
         * - node is not the root (parent is not NULL)
         * - All leaf paths going through parent and node have a
         *   black node count that is 1 lower than other leaf paths.
         */
        sibling = parent->rb_right;
        if (node != sibling) {    /* node == parent->rb_left */
            if (rb_is_red(sibling)) {
                /*
                 * Case 1 - left rotate at parent
                 *
                 *     P               S
                 *    / \             / \
                 *   N   s    -->    p   Sr
                 *      / \         / \
                 *     Sl  Sr      N   Sl
                 */
                tmp1 = sibling->rb_left;
                parent->rb_right = tmp1;
                sibling->rb_left = parent;
                rb_set_parent_color(tmp1, parent, RB_BLACK);
                __rb_rotate_set_parents(parent, sibling, root,
                            RB_RED);
                sibling = tmp1;
            }
            tmp1 = sibling->rb_right;
            if (!tmp1 || rb_is_black(tmp1)) {
                tmp2 = sibling->rb_left;
                if (!tmp2 || rb_is_black(tmp2)) {
                    /*
                     * Case 2 - sibling color flip
                     * (p could be either color here)
                     *
                     *    (p)           (p)
                     *    / \           / \
                     *   N   S    -->  N   s
                     *      / \           / \
                     *     Sl  Sr        Sl  Sr
                     *
                     * This leaves us violating 5) which
                     * can be fixed by flipping p to black
                     * if it was red, or by recursing at p.
                     * p is red when coming from Case 1.
                     */
                    rb_set_parent_color(sibling, parent,
                                RB_RED);
                    if (rb_is_red(parent))
                        rb_set_black(parent);
                    else {
                        node = parent;
                        parent = rb_parent(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /*
                 * Case 3 - right rotate at sibling
                 * (p could be either color here)
                 *
                 *   (p)           (p)
                 *   / \           / \
                 *  N   S    -->  N   sl
                 *     / \             \
                 *    sl  Sr            S
                 *                       \
                 *                        Sr
                 *
                 * Note: p might be red, and then both
                 * p and sl are red after rotation(which
                 * breaks property 4). This is fixed in
                 * Case 4 (in __rb_rotate_set_parents()
                 *         which set sl the color of p
                 *         and set p RB_BLACK)
                 *
                 *   (p)            (sl)
                 *   / \            /  \
                 *  N   sl   -->   P    S
                 *       \        /      \
                 *        S      N        Sr
                 *         \
                 *          Sr
                 */
                tmp1 = tmp2->rb_right;
                sibling->rb_left = tmp1;
                tmp2->rb_right = sibling;
                parent->rb_right = tmp2;
                if (tmp1)
                    rb_set_parent_color(tmp1, sibling,
                                RB_BLACK);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /*
             * Case 4 - left rotate at parent + color flips
             * (p and sl could be either color here.
             *  After rotation, p becomes black, s acquires
             *  p's color, and sl keeps its color)
             *
             *      (p)             (s)
             *      / \             / \
             *     N   S     -->   P   Sr
             *        / \         / \
             *      (sl) sr      N  (sl)
             */
            tmp2 = sibling->rb_left;
            parent->rb_right = tmp2;
            sibling->rb_left = parent;
            rb_set_parent_color(tmp1, sibling, RB_BLACK);
            if (tmp2)
                rb_set_parent(tmp2, parent);
            __rb_rotate_set_parents(parent, sibling, root,
                        RB_BLACK);
            break;
        } else {
            sibling = parent->rb_left;
            if (rb_is_red(sibling)) {
                /* Case 1 - right rotate at parent */
                tmp1 = sibling->rb_right;
                parent->rb_left = tmp1;
                sibling->rb_right = parent;
                rb_set_parent_color(tmp1, parent, RB_BLACK);
                __rb_rotate_set_parents(parent, sibling, root,
                            RB_RED);
                sibling = tmp1;
            }
            tmp1 = sibling->rb_left;
            if (!tmp1 || rb_is_black(tmp1)) {
                tmp2 = sibling->rb_right;
                if (!tmp2 || rb_is_black(tmp2)) {
                    /* Case 2 - sibling color flip */
                    rb_set_parent_color(sibling, parent,
                                RB_RED);
                    if (rb_is_red(parent))
                        rb_set_black(parent);
                    else {
                        node = parent;
                        parent = rb_parent(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /* Case 3 - left rotate at sibling */
                tmp1 = tmp2->rb_left;
                sibling->rb_right = tmp1;
                tmp2->rb_left = sibling;
                parent->rb_left = tmp2;
                if (tmp1)
                    rb_set_parent_color(tmp1, sibling,
                                RB_BLACK);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /* Case 4 - right rotate at parent + color flips */
            tmp2 = sibling->rb_right;
            parent->rb_left = tmp2;
            sibling->rb_right = parent;
            rb_set_parent_color(tmp1, sibling, RB_BLACK);
            if (tmp2)
                rb_set_parent(tmp2, parent);
            __rb_rotate_set_parents(parent, sibling, root,
                        RB_BLACK);
            break;
        }
    }
}

static struct eh_rbtree_node *
__rb_del(struct eh_rbtree_node *node, struct eh_rbtree_root *root)
{
    struct eh_rbtree_node *child = node->rb_right;
    struct eh_rbtree_node *tmp = node->rb_left;
    struct eh_rbtree_node *parent, *rebalance;
    parent_node_t pc;

    if (!tmp) {
        /*
         * Case 1: node to erase has no more than 1 child (easy!)
         *
         * Note that if there is one child it must be red due to 5)
         * and node must be black due to 4). We adjust colors locally
         * so as to bypass __rb_erase_color() later on.
         */
        pc = node->parent_and_color;
        parent = __rb_parent(pc);
        __rb_change_child(node, child, parent, root);
        if (child) {
            child->parent_and_color = pc;
            rebalance = NULL;
        } else
            rebalance = __rb_is_black(pc) ? parent : NULL;
        tmp = parent;
    } else if (!child) {
        /* Still case 1, but this time the child is node->rb_left */
        tmp->parent_and_color = pc = node->parent_and_color;
        parent = __rb_parent(pc);
        __rb_change_child(node, tmp, parent, root);
        rebalance = NULL;
        tmp = parent;
    } else {
        struct eh_rbtree_node *successor = child, *child2;

        tmp = child->rb_left;
        if (!tmp) {
            /*
             * Case 2: node's successor is its right child
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (s)  ->  (x) (c)
             *        \
             *        (c)
             */
            parent = successor;
            child2 = successor->rb_right;

        } else {
            /*
             * Case 3: node's successor is leftmost under
             * node's right child subtree
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (y)  ->  (x) (y)
             *      /            /
             *    (p)          (p)
             *    /            /
             *  (s)          (c)
             *    \
             *    (c)
             */
            do {
                parent = successor;
                successor = tmp;
                tmp = tmp->rb_left;
            } while (tmp);
            child2 = successor->rb_right;
            parent->rb_left = child2;
            successor->rb_right = child;
            rb_set_parent(child, successor);
        }

        tmp = node->rb_left;
        successor->rb_left = tmp;
        rb_set_parent(tmp, successor);

        pc = node->parent_and_color;
        tmp = __rb_parent(pc);
        __rb_change_child(node, successor, tmp, root);

        if (child2) {
            rb_set_parent_color(child2, parent, RB_BLACK);
            rebalance = NULL;
        } else {
            rebalance = rb_is_black(successor) ? parent : NULL;
        }
        successor->parent_and_color = pc;
        tmp = successor;
    }

    return rebalance;
}
/* Non-inline version for rb_erase_augmented() use */
void __rb_erase_color(struct eh_rbtree_node *parent, struct eh_rbtree_root *root)
{
    ____rb_erase_color(parent, root);
}


static void rb_insert_color(struct eh_rbtree_node *node, struct eh_rbtree_root *root)
{
    __rb_insert(node, root);
}

struct eh_rbtree_node * eh_rb_del(struct eh_rbtree_node *node, struct eh_rbtree_root *root)
{
    struct eh_rbtree_node *rebalance;
    struct eh_rbtree_node *leftmost = NULL;

    if (root->rb_leftmost == node)
        leftmost = root->rb_leftmost = eh_rb_next(node);
    rebalance = __rb_del(node, root);
    if (rebalance)
        ____rb_erase_color(rebalance, root);
    return leftmost;
}

struct eh_rbtree_node *eh_rb_last(const struct eh_rbtree_root *root)
{
    struct eh_rbtree_node    *n;

    n = root->rb_node;
    if (!n)
        return NULL;
    while (n->rb_right)
        n = n->rb_right;
    return n;
}

struct eh_rbtree_node *eh_rb_next(const struct eh_rbtree_node *node)
{
    struct eh_rbtree_node *parent;

    if (eh_rb_node_is_empty(node))
        return NULL;

    /*
     * If we have a right-hand child, go down and then left as far
     * as we can.
     */
    if (node->rb_right) {
        node = node->rb_right;
        while (node->rb_left)
            node = node->rb_left;
        return (struct eh_rbtree_node *)node;
    }

    /*
     * No right-hand children. Everything down and left is smaller than us,
     * so any 'next' node must be in the general direction of our parent.
     * Go up the tree; any time the ancestor is a right-hand child of its
     * parent, keep going up. First time it's a left-hand child of its
     * parent, said parent is our 'next' node.
     */
    while ((parent = rb_parent(node)) && node == parent->rb_right)
        node = parent;

    return parent;
}

struct eh_rbtree_node *eh_rb_prev(const struct eh_rbtree_node *node)
{
    struct eh_rbtree_node *parent;

    if (eh_rb_node_is_empty(node))
        return NULL;

    /*
     * If we have a left-hand child, go down and then right as far
     * as we can.
     */
    if (node->rb_left) {
        node = node->rb_left;
        while (node->rb_right)
            node = node->rb_right;
        return (struct eh_rbtree_node *)node;
    }

    /*
     * No left-hand children. Go up till we find an ancestor which
     * is a right-hand child of its parent.
     */
    while ((parent = rb_parent(node)) && node == parent->rb_left)
        node = parent;

    return parent;
}

void rb_replace_node(struct eh_rbtree_node *victim, struct eh_rbtree_node *new,
             struct eh_rbtree_root *root)
{
    struct eh_rbtree_node *parent = rb_parent(victim);

    /* Copy the pointers/colour from the victim to the replacement */
    *new = *victim;

    /* Set the surrounding nodes to point to the replacement */
    if (victim->rb_left)
        rb_set_parent(victim->rb_left, new);
    if (victim->rb_right)
        rb_set_parent(victim->rb_right, new);
    __rb_change_child(victim, new, parent, root);
}


struct eh_rbtree_node * eh_rb_add(struct eh_rbtree_node *node, struct eh_rbtree_root *tree)
{
    struct eh_rbtree_node **link = &tree->rb_node;
    struct eh_rbtree_node *parent = NULL;
    bool leftmost = true;

    while (*link) {
        parent = *link;
        if (tree->cmp(node, parent) < 0) {
            link = &parent->rb_left;
        } else {
            link = &parent->rb_right;
            leftmost = false;
        }
    }

    rb_link_node(node, parent, link);
    if (leftmost)
        tree->rb_leftmost = node;
    rb_insert_color(node, tree);
    
    return leftmost ? node : NULL;
}


struct eh_rbtree_node *eh_rb_find_add(struct eh_rbtree_node *node, struct eh_rbtree_root *tree)
{
    struct eh_rbtree_node **link = &tree->rb_node;
    struct eh_rbtree_node *parent = NULL;
    bool leftmost = true;
    int c;

    while (*link) {
        parent = *link;
        c = tree->cmp(node, parent);

        if (c < 0)
            link = &parent->rb_left;
        else if (c > 0){
            link = &parent->rb_right;
            leftmost = false;
        }else
            return parent;
    }

    rb_link_node(node, parent, link);
    if (leftmost)
        tree->rb_leftmost = node;
    rb_insert_color(node, tree);
    return node;
}

struct eh_rbtree_node *eh_rb_find_new_add(const void *key, struct eh_rbtree_root *tree,
          int (*cmp)(const void *key, const struct eh_rbtree_node *), void *user_data,
          struct eh_rbtree_node* (new_node)(void *user_data))
{
    struct eh_rbtree_node **link = &tree->rb_node;
    struct eh_rbtree_node *parent = NULL;
    struct eh_rbtree_node *node; 
    bool leftmost = true;
    int c;

    while (*link) {
        parent = *link;
        c = cmp(key, parent);

        if (c < 0)
            link = &parent->rb_left;
        else if (c > 0){
            link = &parent->rb_right;
            leftmost = false;
        }else
            return parent;
    }
    node = new_node(user_data);
    if(!node)
        return NULL;
    rb_link_node(node, parent, link);
    if (leftmost)
        tree->rb_leftmost = node;
    rb_insert_color(node, tree);
    return node;
}


struct eh_rbtree_node * eh_rb_match_find(const void *key, struct eh_rbtree_root *tree, 
    int (*match)(const void *key, const struct eh_rbtree_node *)){
    struct eh_rbtree_node *node;
    int c;

    node = tree->rb_node;
    while(node){
        c = match(key, node);
        if(c < 0){
            node = node->rb_left;
        }else if(c > 0){
            node = node->rb_right;
        }else{
            return node;
        }
    }
    return NULL;
}


struct eh_rbtree_node *eh_rb_find_first(const void *key, const struct eh_rbtree_root *tree,
          int (*match)(const void *key, const struct eh_rbtree_node *))
{
    struct eh_rbtree_node *node = tree->rb_node;
    struct eh_rbtree_node *match_node = NULL;

    while (node) {
        int c = match(key, node);

        if (c <= 0) {
            if (!c)
                match_node = node;
            node = node->rb_left;
        } else if (c > 0) {
            node = node->rb_right;
        }
    }

    return match_node;
}

struct eh_rbtree_node *rb_next_postorder(const struct eh_rbtree_node *node)
{
    const struct eh_rbtree_node *parent;
    if (!node)
        return NULL;
    parent = rb_parent(node);

    /* If we're sitting on node, we've already seen our children */
    if (parent && node == parent->rb_left && parent->rb_right) {
        /* If we are the parent's left node, go to the parent's right
         * node then all the way down to the left */
        return rb_left_deepest_node(parent->rb_right);
    } else
        /* Otherwise we are the parent's right node, and the parent
         * should be next */
        return (struct eh_rbtree_node *)parent;
}

struct eh_rbtree_node *rb_first_postorder(const struct eh_rbtree_root *root)
{
    if (!root->rb_node)
        return NULL;

    return rb_left_deepest_node(root->rb_node);
}

struct eh_rbtree_node *eh_rb_next_match(const void *key, struct eh_rbtree_node *node,
          int (*match)(const void *key, const struct eh_rbtree_node *))
{
    node = eh_rb_next(node);
    if (node && match(key, node))
        node = NULL;
    return node;
}