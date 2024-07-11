package com.oppo.vlr.entity;


import lombok.Data;
import java.util.List;

@Data
public class GModel {
    private String taskID;

    private String round;

    private String syntax;

    private String ciphertext;

    private String auth_tag;

    private String sig;

    private String index;
}
