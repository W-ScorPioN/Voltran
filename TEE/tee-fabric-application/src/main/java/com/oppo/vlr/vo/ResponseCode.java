package com.oppo.vlr.vo;

/**
 * 响应码枚举对象。 和http状态码相互独立
 *
 * @author dragon
 */

public enum ResponseCode {
    //通用
    OK(2000, "Success"),
    ERROR(20001, "Error"),
    //熔断专用
    REMOTE_CALL_FAIL(5000, "服务暂时不可用:"),
    UNKNOWN_EXCEPTION(505, "系统异常"),
    BAD_REQUEST(4000, "参数错误"),
    ;


    private final int value;
    private final String description;

    ResponseCode(int value, String description) {
        this.value = value;
        this.description = description;
    }

    public int value() {
        return this.value;
    }

    public String getDescription() {
        return this.description;
    }

    public static ResponseCode valueOf(int code) {
        for (ResponseCode responseCode : values()) {
            if (responseCode.value == code) {
                return responseCode;
            }
        }
        throw new IllegalArgumentException("No matching constant for [" + code + "]");
    }

    @Override
    public String toString() {
        return Integer.toString(this.value);
    }

}
