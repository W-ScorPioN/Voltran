package com.oppo.vlr.exception;

import com.google.gson.Gson;
import com.google.gson.JsonSyntaxException;
// import com.oppo.vlr.service.impl.BusinessServiceImpl;
import com.oppo.vlr.vo.ResultBody;
import lombok.extern.slf4j.Slf4j;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.hyperledger.fabric.gateway.ContractException;
import org.hyperledger.fabric.sdk.ProposalResponse;
import org.springframework.web.bind.MethodArgumentNotValidException;
import org.springframework.web.bind.annotation.ControllerAdvice;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.ResponseBody;

import javax.servlet.http.HttpServletRequest;

//@Slf4j
@ControllerAdvice
public class GlobalExceptionHandler {
    Logger logger = LogManager.getLogger(GlobalExceptionHandler.class);

    /**
     * 处理自定义的业务异常
     * @param req
     * @param e
     * @return
     */
    @ExceptionHandler(value = BizException.class)
    @ResponseBody
    public  ResultBody bizExceptionHandler(HttpServletRequest req, BizException e){
        logger.error("发生业务异常！原因是：{}",e.getErrorMsg());
        return ResultBody.error(e.getErrorCode(),e.getErrorMsg());
    }

    /**
     * 处理空指针的异常
     * @param req
     * @param e
     * @return
     */
    @ExceptionHandler(value =NullPointerException.class)
    @ResponseBody
    public ResultBody exceptionHandler(HttpServletRequest req, NullPointerException e){
        logger.error("发生空指针异常！原因是:",e.getMessage());
        return ResultBody.error(CommonEnum.BODY_NOT_MATCH);
    }

    @ExceptionHandler(value = ContractException.class)
    @ResponseBody
    public ResultBody exceptionHandler(HttpServletRequest req, ContractException e){
        logger.error("发送合约异常！原因是:{}",e.getMessage());
        for(ProposalResponse response : e.getProposalResponses()) {
            Gson gson = new Gson();
            try{
                ChainException ex = gson.fromJson(response.getMessage(), ChainException.class);
                return ResultBody.error(ex.getCode(), ex.getMessage());
            } catch (JsonSyntaxException jsonEx) {
            }
            return ResultBody.error(response.getMessage());
        }
        return ResultBody.error(e.getMessage());
    }

    /**
     * 处理其他异常
     * @param req
     * @param e
     * @return
     */
    @ExceptionHandler(value = MethodArgumentNotValidException.class)
    @ResponseBody
    public ResultBody exceptionHandler(HttpServletRequest req, MethodArgumentNotValidException e){
        logger.error("参数校验异常：{}", e.getMessage());
        return ResultBody.error(CommonEnum.BODY_NOT_MATCH.getResultCode(), e.getMessage());
    }

    /**
     * 处理其他异常
     * @param req
     * @param e
     * @return
     */
    @ExceptionHandler(value =Exception.class)
    @ResponseBody
    public ResultBody exceptionHandler(HttpServletRequest req, Exception e){
        logger.error("未知异常：{}", e.getMessage());
        return ResultBody.error(CommonEnum.INTERNAL_SERVER_ERROR.getResultCode(), e.getMessage());
    }
}
