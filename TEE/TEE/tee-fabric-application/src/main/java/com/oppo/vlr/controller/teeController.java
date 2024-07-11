package com.oppo.vlr.controller;

import com.oppo.vlr.entity.GModel;
import com.oppo.vlr.entity.TaskLog;
// import com.oppo.vlr.service.IBusinessService;
import com.oppo.vlr.vo.ResultBody;
import io.swagger.annotations.Api;
import io.swagger.annotations.ApiOperation;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.hyperledger.fabric.gateway.Contract;
import org.hyperledger.fabric.gateway.ContractException;
import org.hyperledger.fabric.gateway.Network;
import org.hyperledger.fabric.sdk.Peer;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RestController;

import javax.annotation.Resource;
import java.nio.charset.StandardCharsets;
import java.util.EnumSet;
import java.util.concurrent.TimeoutException;
import com.google.gson.Gson;

@Api(tags = "tee数据上链接口")
@RestController

public class teeController {
    Logger logger = LogManager.getLogger(teeController.class);
    @Resource
    @Qualifier("tee")
    private Contract contract;
    @Resource
    private Network network;

    @ApiOperation("上传PK")
    @PostMapping("/tee/testPK")
    public ResultBody testPK(@RequestBody TaskLog taskLog) throws ContractException, InterruptedException, TimeoutException {
        logger.info("/tee/uploadPK\n{}", taskLog);

        //返回的是交易id
        return ResultBody.success("test success!");
    }

    @ApiOperation("上传PK")
    @PostMapping("/tee/uploadPK")
    public ResultBody uploadPK(@RequestBody TaskLog taskLog) throws ContractException, InterruptedException, TimeoutException {
        logger.info("/tee/uploadPK\n{}", taskLog);

        byte[] invokeResult = contract.createTransaction("uploadPK")
                .setEndorsingPeers(network.getChannel().getPeers(EnumSet.of(Peer.PeerRole.ENDORSING_PEER)))
                .submit(taskLog.getTaskid(), taskLog.getPk_x(), taskLog.getPk_y());

        //返回的是交易id
        return ResultBody.success(new String(invokeResult, StandardCharsets.UTF_8));
    }

    @ApiOperation("获取PK")
    @PostMapping("/tee/getPK")
    public ResultBody getPK(@RequestBody GModel gModel) throws ContractException {
        logger.info("/tee/getPK\n{}", gModel);

        //调用区块链
        byte[] queryAResultBefore = contract.evaluateTransaction("GetPK", gModel.getTaskID(), gModel.getSyntax());
        return ResultBody.success(new String(queryAResultBefore, StandardCharsets.UTF_8));
    }

    @ApiOperation("上传globalModel")
    @PostMapping("/tee/uploadGlobalModel")
    public ResultBody uploadGlobalModel(@RequestBody GModel gModel) throws ContractException, InterruptedException, TimeoutException {
        logger.info("/tee/uploadGlobalModel\n{}", gModel);

//        Gson gson = new Gson();
        byte[] invokeResult = contract.createTransaction("uploadGlobalModel")
                .setEndorsingPeers(network.getChannel().getPeers(EnumSet.of(Peer.PeerRole.ENDORSING_PEER)))
                .submit(gModel.getTaskID(), gModel.getRound(), gModel.getCiphertext(), gModel.getAuth_tag(), gModel.getSig(), gModel.getIndex());

        //返回的是交易id
        return ResultBody.success(new String(invokeResult, StandardCharsets.UTF_8));
    }

    @ApiOperation("获取Ciphertext")
    @PostMapping("/tee/getCiphertext")
    public ResultBody getCiphertext(@RequestBody GModel gModel) throws ContractException {
        logger.info("/tee/getCiphertext\n{}", gModel);

        //调用区块链
        byte[] queryAResultBefore = contract.evaluateTransaction("getCiphertext", gModel.getTaskID(),gModel.getRound(), gModel.getIndex());
        return ResultBody.success(new String(queryAResultBefore, StandardCharsets.UTF_8));
    }

    @ApiOperation("获取Auth_tag")
    @PostMapping("/tee/getAuth_tag")

    public ResultBody getAuth_tag(@RequestBody GModel gModel) throws ContractException {
        logger.info("/tee/getAuth_tag\n{}", gModel);

        //调用区块链
        byte[] queryAResultBefore = contract.evaluateTransaction("getAuth_tag", gModel.getTaskID(),gModel.getRound(), gModel.getIndex());
        return ResultBody.success(new String(queryAResultBefore, StandardCharsets.UTF_8));
    }

    // @ApiOperation("上传y_Minus_Y_hat_Commitment")
    // @PostMapping("/vlr2party/uploadY_Minus_Y_hat_Commitment")
    // public ResultBody uploadY_Minus_Y_hat_Commitment(@RequestBody TaskLog taskLog) throws ContractException, InterruptedException, TimeoutException {
    //     logger.info("/vlr2party/uploadY_Minus_Y_hat_Commitment\n{}", taskLog);

    //     byte[] invokeResult = contract.createTransaction("UploadY_Minus_Y_hat_Commitment")
    //             .setEndorsingPeers(network.getChannel().getPeers(EnumSet.of(Peer.PeerRole.ENDORSING_PEER)))
    //             .submit(taskLog.getTaskid(), taskLog.getRound(), taskLog.getY_Minus_Y_hat_Commitment());

    //     //返回的是交易id
    //     return ResultBody.success(new String(invokeResult, StandardCharsets.UTF_8));
    // }

    // @ApiOperation("获取y_Minus_Y_hat_Commitment")
    // @PostMapping("/vlr2party/getY_Minus_Y_hat_Commitment")
    // public ResultBody getY_Minus_Y_hat_Commitment(@RequestBody TaskLog taskLog) throws ContractException {
    //     logger.info("/vlr2party/getY_Minus_Y_hat_Commitment\n{}", taskLog);

    //     //调用区块链
    //     byte[] queryAResultBefore = contract.evaluateTransaction("GetY_Minus_Y_hat_Commitment", taskLog.getTaskid(),taskLog.getRound());
    //     return ResultBody.success(new String(queryAResultBefore, StandardCharsets.UTF_8));
    // }

    // @ApiOperation("上传x_i_B_Commitment")
    // @PostMapping("/vlr2party/uploadX_i_B_Commitment")
    // public ResultBody uploadX_i_B_Commitment(@RequestBody TaskLog taskLog) throws ContractException, InterruptedException, TimeoutException {
    //     logger.info("/vlr2party/uploadX_i_B_Commitment\n{}", taskLog);

    //     byte[] invokeResult = contract.createTransaction("uploadX_i_B_Commitment")
    //             .setEndorsingPeers(network.getChannel().getPeers(EnumSet.of(Peer.PeerRole.ENDORSING_PEER)))
    //             .submit(taskLog.getTaskid(), taskLog.getRound(), taskLog.getX_i_B_Commitment());

    //     //返回的是交易id
    //     return ResultBody.success(new String(invokeResult, StandardCharsets.UTF_8));
    // }

    // @ApiOperation("获取x_i_B_Commitment")
    // @PostMapping("/vlr2party/getx_i_B_Commitment")
    // public ResultBody getX_i_B_Commitment(@RequestBody TaskLog taskLog) throws ContractException {
    //     logger.info("//vlr2party/getX_i_B_Commitment\n{}", taskLog);

    //     //调用区块链
    //     byte[] queryAResultBefore = contract.evaluateTransaction("GetX_i_B_Commitment", taskLog.getTaskid(),taskLog.getRound());
    //     return ResultBody.success(new String(queryAResultBefore, StandardCharsets.UTF_8));
    // }

    // @ApiOperation("上传output_Commitment")
    // @PostMapping("/vlr2party/uploadOutput_Commitment")
    // public ResultBody uploadOutput_Commitment(@RequestBody TaskLog taskLog) throws ContractException, InterruptedException, TimeoutException {
    //     logger.info("/vlr2party/uploadOutput_Commitment\n{}", taskLog);

    //     byte[] invokeResult = contract.createTransaction("uploadOutput_Commitment")
    //             .setEndorsingPeers(network.getChannel().getPeers(EnumSet.of(Peer.PeerRole.ENDORSING_PEER)))
    //             .submit(taskLog.getTaskid(), taskLog.getRound(), taskLog.getOutput_Commitment());

    //     //返回的是交易id
    //     return ResultBody.success(new String(invokeResult, StandardCharsets.UTF_8));
    // }

    // @ApiOperation("获取output_Commitment")
    // @PostMapping("/vlr2party/getOutput_Commitment")
    // public ResultBody getOutput_Commitment(@RequestBody TaskLog taskLog) throws ContractException {
    //     logger.info("//vlr2party/getOutput_Commitment\n{}", taskLog);

    //     //调用区块链
    //     byte[] queryAResultBefore = contract.evaluateTransaction("GetOutput_Commitment", taskLog.getTaskid(),taskLog.getRound());
    //     return ResultBody.success(new String(queryAResultBefore, StandardCharsets.UTF_8));
    // }
}
