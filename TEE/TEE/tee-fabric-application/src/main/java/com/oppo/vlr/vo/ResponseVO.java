package com.oppo.vlr.vo;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;
import java.io.Serializable;

@Data
@AllArgsConstructor
@NoArgsConstructor
@JsonInclude(JsonInclude.Include.NON_NULL)
public class ResponseVO<T> implements Serializable {

    private static final long serialVersionUID = 1L;

    private int code;

    private String message;

    private T data;

    private Long timestamp = System.currentTimeMillis();

    public ResponseVO(ResponseCode responseCode) {
        this.code = responseCode.value();
        this.message = responseCode.getDescription();
    }

    public ResponseVO(int code, String msg) {
        this.code = code;
        this.message = msg;
        this.timestamp = System.currentTimeMillis();
    }


    public ResponseVO(ResponseCode responseCode, T data) {
        this.code = responseCode.value();
        this.message = responseCode.getDescription();
        this.data = data;
        this.timestamp = System.currentTimeMillis();
    }

}
