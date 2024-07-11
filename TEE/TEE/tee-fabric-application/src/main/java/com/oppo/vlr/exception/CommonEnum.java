package com.oppo.vlr.exception;

public enum CommonEnum implements BaseErrorInfoInterface {
    // 数据操作错误定义
    SUCCESS("200", "成功"),
    BODY_NOT_MATCH("500","请求的数据格式不符"),
    INTERNAL_SERVER_ERROR("501", "服务器内部错误"),
    REPEATED_SUBMIT("502", "重复提交数据"),
    NOT_FOUND("404", "未找到该资源")
    ;

    /** 错误码 */
    private String resultCode;

    /** 错误描述 */
    private String resultMsg;

    CommonEnum(String resultCode, String resultMsg) {
        this.resultCode = resultCode;
        this.resultMsg = resultMsg;
    }

    @Override
    public String getResultCode() {
        return resultCode;
    }

    @Override
    public String getResultMsg() {
        return resultMsg;
    }
}
