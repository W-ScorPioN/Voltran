package com.oppo.vlr.entity;

import lombok.Data;

import javax.validation.constraints.NotBlank;

@Data
public class TaskLog {
    @NotBlank(message = "taskid")
    private String taskid;          //日志对应的state为空

    @NotBlank(message = "round为空")
    private String round;         //日志运行的round

    @NotBlank(message = "pk为空")
    private String pk_x;
    private String pk_y;
    private String syntax;
    private String globalModel;
//    private String theta_B_Mul_x_i_B_Commitment;
//    private String y_Minus_Y_hat_Commitment;
//    private String x_i_B_Commitment;
//    private String output_Commitment;

//    @NotBlank(message = "state")
//    private String state;          //日志对应的state为空
//    private String commit_B1;
//    private String b1;
//    private String commit_A1;
//    private String a1;
//    private String commit_B2;
//    private String b2;
//    private String commit_A2;
//    private String a2;

    //private int status;             //是否已上链
}
