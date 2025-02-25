// This file is licensed under the Elastic License 2.0. Copyright 2021-present, StarRocks Limited.
package com.starrocks.sql.ast;

import com.starrocks.analysis.Expr;
import com.starrocks.analysis.JoinOperator;

import java.util.List;

public class JoinRelation extends Relation {
    private final JoinOperator type;
    private Relation left;
    private Relation right;
    private Expr onPredicate;
    private String joinHint = "";
    private final boolean lateral;

    /**
     * usingColNames is created by parser
     * and will be converted to onPredicate in Analyzer
     */
    private List<String> usingColNames;

    public JoinRelation(JoinOperator type, Relation left, Relation right, Expr onPredicate, boolean isLateral) {
        this.type = type;
        this.left = left;
        this.right = right;
        this.onPredicate = onPredicate;
        this.lateral = isLateral;
    }

    public JoinOperator getType() {
        return type;
    }

    public Relation getLeft() {
        return left;
    }

    public Relation getRight() {
        return right;
    }

    public void setLeft(Relation left) {
        this.left = left;
    }

    public void setRight(Relation right) {
        this.right = right;
    }

    public Expr getOnPredicate() {
        return onPredicate;
    }

    public void setOnPredicate(Expr onPredicate) {
        this.onPredicate = onPredicate;
    }

    public void setJoinHint(String joinHint) {
        this.joinHint = joinHint;
    }

    public String getJoinHint() {
        return joinHint;
    }

    public boolean isLateral() {
        return lateral;
    }

    public List<String> getUsingColNames() {
        return usingColNames;
    }

    public void setUsingColNames(List<String> usingColNames) {
        this.usingColNames = usingColNames;
    }

    @Override
    public <R, C> R accept(AstVisitor<R, C> visitor, C context) {
        return visitor.visitJoin(this, context);
    }
}