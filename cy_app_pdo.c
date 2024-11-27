/***************************************************************************//**
* \file cy_app_pdo.c
* \version 2.0
*
* \brief
* Implements functions associated source capability (PDO) evaluation
* functions.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "cy_app_config.h"
#include "cy_pdstack_common.h"
#include "cy_pdstack_dpm.h"
#include "cy_pdutils.h"
#include "cy_app_pdo.h"
#include "cy_app.h"

#if (!(CY_PD_SOURCE_ONLY))

/* PDO Variables. */
uint32_t glAppMaxMinPower[NO_OF_TYPEC_PORTS];
uint32_t glAppContractPower[NO_OF_TYPEC_PORTS];
uint32_t glAppContractVoltage[NO_OF_TYPEC_PORTS];
uint32_t glAppOperCurPower[NO_OF_TYPEC_PORTS];

#if CY_PD_EPR_ENABLE
uint32_t glAppPdoPower[NO_OF_TYPEC_PORTS];
#endif /* CY_PD_EPR_ENABLE */

static uint32_t calc_power(uint32_t voltage, uint32_t current)
{
    /*
       Voltage is expressed in 50 mV unit
       Current is expressed in 10 mA unit
       Power is expressed in 250 mW unit
       */
    return (CY_PDUTILS_DIV_ROUND_UP(voltage * current, 500));
}

static uint32_t calc_current(uint32_t power, uint32_t voltage)
{
    /*
       Power is expressed in 250 mW unit
       Voltage is expressed in 50 mV unit
       Current is expressed in 10 mA unit
       */
    return (CY_PDUTILS_DIV_ROUND_UP(power * 500, voltage));
}

/**
 * Checks if SRC pdo is acceptable for SNK pdo
 * @param context Pointer to the PDStack context
 * @param pdo_src pointer to current SRC PDO
 * @param snk_pdo_idx Index to the sink PDO
 * @return True if current src PDO is acceptable for current sink PDO
 */
static bool is_src_acceptable_snk(cy_stc_pdstack_context_t* context, cy_pd_pd_do_t* pdo_src, uint8_t snk_pdo_idx)
{
    cy_pd_pd_do_t* pdo_snk = (cy_pd_pd_do_t*)&(context->dpmStat.curSnkPdo[snk_pdo_idx]);
    uint8_t port = context->port;

    uint32_t snk_supply_type = pdo_snk->fixed_snk.supplyType;
    uint32_t fix_volt;
    uint32_t maxVolt = 0u;
    uint32_t minVolt = 0u;
    uint32_t out = false;
    uint32_t max_min_temp, compare_temp;
    uint32_t oper_cur_pwr;

    max_min_temp = context->dpmStat.curSnkMaxMin[snk_pdo_idx] & CY_PD_SNK_MIN_MAX_MASK;
#if CY_PD_EPR_ENABLE
    glAppPdoPower[port] = 0;
#endif /* CY_PD_EPR_ENABLE */

    switch(pdo_src->fixed_src.supplyType)
    {
        case CY_PDSTACK_PDO_FIXED_SUPPLY:  /* Fixed supply PDO */
            fix_volt = pdo_src->fixed_src.voltage;
#if CY_PD_EPR_ENABLE
            glAppPdoPower[port] = calc_power(fix_volt, pdo_src->fixed_src.maxCurrent);
#endif /* CY_PD_EPR_ENABLE */
            maxVolt = Cy_PdUtils_DivRoundUp(fix_volt, 20);
            minVolt = fix_volt - maxVolt;
            maxVolt = fix_volt + maxVolt;

            switch(snk_supply_type)  /* Checking sink PDO type */
            {
                case CY_PDSTACK_PDO_FIXED_SUPPLY:
                    if(fix_volt == pdo_snk->fixed_snk.voltage)
                    {
                        compare_temp = CY_PDUTILS_GET_MAX (max_min_temp, pdo_snk->fixed_snk.opCurrent);
                        if (pdo_src->fixed_src.maxCurrent >= compare_temp)
                        {
                            glAppOperCurPower[port] = pdo_snk->fixed_snk.opCurrent;
                            out = true;
                        }
                    }
                    break;

                case CY_PDSTACK_PDO_VARIABLE_SUPPLY:
                    if ((minVolt >= pdo_snk->var_snk.minVoltage) && (maxVolt <= pdo_snk->var_snk.maxVoltage))
                    {
                        compare_temp = CY_PDUTILS_GET_MAX (max_min_temp, pdo_snk->var_snk.opCurrent);
                        if (pdo_src->fixed_src.maxCurrent >= compare_temp)
                        {
                            glAppOperCurPower[port] = pdo_snk->var_snk.opCurrent;
                            out = true;
                        }
                    }
                    break;

                case CY_PDSTACK_PDO_BATTERY:
                    if ((minVolt >= pdo_snk->bat_snk.minVoltage) && (maxVolt <= pdo_snk->bat_snk.maxVoltage))
                    {
                        fix_volt = minVolt;

                        /* Calculate the operating current and min/max current values */
                        oper_cur_pwr = calc_current(pdo_snk->bat_snk.opPower, minVolt);
                        /* Calculate the operating current and min/max current values */
                        /* Intel: Always set operating current to 27 W/fixed_pdo_voltage and max op. current to PDO voltage. */
                        max_min_temp = calc_current(max_min_temp, minVolt);

                        /* Make sure the source can supply the maximum current that may be required. */
                        compare_temp = CY_PDUTILS_GET_MAX(max_min_temp, oper_cur_pwr);
                        if (pdo_src->fixed_src.maxCurrent >= compare_temp)
                        {
                            glAppOperCurPower[port] = oper_cur_pwr;
                            out = true;
                        }
                    }
                    break;

                default:
                    break;
            }

            if (out)
            {
                glAppContractVoltage[port] = fix_volt;
                glAppContractPower[port]   = calc_power (fix_volt, glAppOperCurPower[port]);
                glAppMaxMinPower[port]  = max_min_temp;
            }
            break;

        case CY_PDSTACK_PDO_BATTERY:   /* SRC is a battery */
            maxVolt = pdo_src->bat_src.maxVoltage;
            minVolt = pdo_src->bat_src.minVoltage;
#if CY_PD_EPR_ENABLE
            glAppPdoPower[port] = pdo_src->bat_src.maxPower;
#endif /* CY_PD_EPR_ENABLE */
            switch(snk_supply_type)
            {
                case CY_PDSTACK_PDO_FIXED_SUPPLY:
                    /* Battery cannot supply fixed voltage
                     * Battery voltage changes with time
                     * This contract if permitted can be un-reliable */
                    break;

                case CY_PDSTACK_PDO_VARIABLE_SUPPLY:
                    if((minVolt >= pdo_snk->var_snk.minVoltage) && (maxVolt <= pdo_snk->var_snk.maxVoltage))
                    {
                        /* Calculate the expected operating power and maximum power requirement */
                        oper_cur_pwr = calc_power(maxVolt, pdo_snk->var_snk.opCurrent);
                        max_min_temp = calc_power(maxVolt, max_min_temp);

                        compare_temp = CY_PDUTILS_GET_MAX (oper_cur_pwr, max_min_temp);
                        if (pdo_src->bat_src.maxPower >= compare_temp)
                        {
                            glAppOperCurPower[port] = oper_cur_pwr;
                            out = true;
                        }
                    }
                    break;

                case CY_PDSTACK_PDO_BATTERY:
                    /* Battery connected directly to a battery.
                     * This combination is unreliable. */
                    if((minVolt >= pdo_snk->bat_snk.minVoltage) && (maxVolt <= pdo_snk->bat_snk.maxVoltage))
                    {
                        compare_temp = CY_PDUTILS_GET_MAX (max_min_temp, pdo_snk->bat_snk.opPower);
                        if (pdo_src->bat_src.maxPower >= compare_temp)
                        {
                            glAppOperCurPower[port] = pdo_snk->bat_snk.opPower;
                            out = true;
                        }
                    }
                    break;

                default:
                    break;
            }

            if (out)
            {
                glAppContractVoltage[port] = maxVolt;
                glAppMaxMinPower[port]  = max_min_temp;
                glAppContractPower[port]   = glAppOperCurPower[port];
            }
            break;

        case CY_PDSTACK_PDO_VARIABLE_SUPPLY:   /* Variable supply PDO */
            maxVolt = pdo_src->var_src.maxVoltage;
            minVolt = pdo_src->var_src.minVoltage;

            switch (snk_supply_type) /* Checking sink PDO type */
            {
                case CY_PDSTACK_PDO_FIXED_SUPPLY:
                    /* This connection is not feasible
                     * A variable source cannot provide a fixed voltage */
                    break;

                case CY_PDSTACK_PDO_VARIABLE_SUPPLY:
                    if((minVolt >= pdo_snk->var_snk.minVoltage) && (maxVolt <= pdo_snk->var_snk.maxVoltage))
                    {
                        compare_temp = CY_PDUTILS_GET_MAX (pdo_snk->var_snk.opCurrent, max_min_temp);

                        if (pdo_src->var_src.maxCurrent >= compare_temp)
                        {
                            glAppContractPower[port] = calc_power(minVolt, pdo_snk->var_snk.opCurrent);
                            glAppOperCurPower[port]   = pdo_snk->var_snk.opCurrent;
                            out = true;
                        }
                    }
                    break;

                case CY_PDSTACK_PDO_BATTERY:
                    if((minVolt >= pdo_snk->bat_snk.minVoltage) && (maxVolt <= pdo_snk->bat_snk.maxVoltage))
                    {
                        /* Convert from power to current */
                        oper_cur_pwr = calc_current(pdo_snk->bat_snk.opPower, minVolt);
                        max_min_temp = calc_current(max_min_temp, minVolt);

                        compare_temp = CY_PDUTILS_GET_MAX (oper_cur_pwr, max_min_temp);
                        if (pdo_src->var_src.maxCurrent >= compare_temp)
                        {
                            glAppContractPower[port] = pdo_snk->bat_snk.opPower;
                            glAppOperCurPower[port]   = oper_cur_pwr;
                            out = true;
                        }
                    }
                    break;

                default:
                    break;
            }

            if (out)
            {
                glAppContractVoltage[port] = maxVolt;
                glAppMaxMinPower[port]  = max_min_temp;
            }
            break;
#if (CY_PD_EPR_AVS_ENABLE)
            case CY_PDSTACK_PDO_AUGMENTED:
                if((pdo_src->pps_src.apdoType == CY_PDSTACK_APDO_AVS) && (snk_pdo_idx >= CY_PD_MAX_NO_OF_PDO))
                {
                    /* Convert voltage to 50 mV from 100 mV unit */
                    maxVolt = pdo_src->epr_avs_src.maxVolt * 2u;
                    minVolt = pdo_src->epr_avs_src.minVolt * 2u;

                    switch(snk_supply_type)
                    {
                        case CY_PDSTACK_PDO_FIXED_SUPPLY:
                            if((minVolt <= pdo_snk->fixed_snk.voltage) && (maxVolt >= pdo_snk->fixed_snk.voltage))
                            {
                                oper_cur_pwr = calc_power(pdo_snk->fixed_snk.voltage, pdo_snk->fixed_snk.opCurrent);
                                max_min_temp = calc_power(pdo_snk->fixed_snk.voltage, max_min_temp);
                                compare_temp = CY_PDUTILS_GET_MAX (max_min_temp, oper_cur_pwr);

                                /* Convert PDP into 250 mW unit */
                                if(pdo_src->epr_avs_src.pdp * 4u >= compare_temp)
                                {
                                    glAppOperCurPower[port] = oper_cur_pwr;
                                    glAppContractVoltage[port] = pdo_snk->fixed_snk.voltage;
                                    out = true;
                                }
                            }
                            break;

                        case CY_PDSTACK_PDO_AUGMENTED:
                            if(pdo_snk->pps_snk.apdoType == CY_PDSTACK_APDO_AVS)
                            {
                                /* Convert voltage to 50 mV from 100 mV unit */
                                if((minVolt <= pdo_snk->epr_avs_snk.minVolt * 2u) && (maxVolt >= pdo_snk->epr_avs_snk.maxVolt * 2u))
                                {
                                    max_min_temp = calc_power(pdo_snk->epr_avs_snk.maxVolt * 2u, max_min_temp);
                                    compare_temp = CY_PDUTILS_GET_MAX (max_min_temp, pdo_snk->epr_avs_snk.pdp * 4u);

                                    if(pdo_src->epr_avs_src.pdp * 4u >= compare_temp)
                                    {
                                        /* Convert PDP into 250 mW unit */
                                        glAppOperCurPower[port] = pdo_snk->epr_avs_snk.pdp * 4u;
                                        glAppContractVoltage[port] = pdo_snk->epr_avs_snk.maxVolt * 2u;
                                        out = true;
                                    }
                                }
                            }
                            break;

                        default:
                            break;
                    }
                }
                if (out)
                {
                    glAppMaxMinPower[port]  = max_min_temp;
                    glAppContractPower[port]   = glAppOperCurPower[port];
                }
                break;
#endif /* (CY_PD_EPR_AVS_ENABLE) */
        default:
            break;
    }

    return out;
}

static cy_pd_pd_do_t form_rdo(cy_stc_pdstack_context_t* context, uint8_t pdo_no, bool capMisMatch, bool giveBack, const cy_stc_pdstack_pd_packet_t* srcCap)
{
#if (CY_PD_REV3_ENABLE)
    const cy_stc_pd_dpm_config_t *dpm = &(context->dpmConfig);
#if (CY_PD_EPR_ENABLE)
    const cy_stc_pdstack_dpm_ext_status_t *dpmExtStat = &(context->dpmExtStat);
#endif /* (CY_PD_EPR_ENABLE) */
#endif /* CY_PD_REV3_ENABLE */

    cy_pd_pd_do_t snkRdo;
    uint8_t port = context->port;

    snkRdo.val = 0u;
    snkRdo.rdo_gen.noUsbSuspend = context->dpmStat.snkUsbSuspEn;
    snkRdo.rdo_gen.usbCommCap = context->dpmStat.snkUsbCommEn;
    snkRdo.rdo_gen.capMismatch = capMisMatch;
#if (CY_PD_EPR_ENABLE)
    /* In request PDO index is SPR 1...7, EPR 8...13 */
    if(pdo_no > CY_PD_MAX_NO_OF_PDO)
    {
        /* if PDO index > 7, set the bit 31 and limit EPR obj_pos in 0...5 range */
        snkRdo.rdo_gen.eprPdo = true;
    }
    snkRdo.rdo_gen.objPos = (pdo_no & CY_PD_MAX_NO_OF_PDO);
#else
    snkRdo.rdo_gen.objPos = pdo_no;
#endif /* CY_PD_EPR_ENABLE */

    if(srcCap->dat[pdo_no - 1u].fixed_src.supplyType != CY_PDSTACK_PDO_AUGMENTED)
    {
        snkRdo.rdo_gen.giveBackFlag = (capMisMatch) ? false : giveBack;
        snkRdo.rdo_gen.opPowerCur = glAppOperCurPower[port];
        snkRdo.rdo_gen.minMaxPowerCur = glAppMaxMinPower[port];
        if (
                (snkRdo.rdo_gen.giveBackFlag == false) &&
                (snkRdo.rdo_gen.opPowerCur > snkRdo.rdo_gen.minMaxPowerCur)
           )
        {
            snkRdo.rdo_gen.minMaxPowerCur = snkRdo.rdo_gen.opPowerCur;
        }
    }
    else
    {
#if (CY_PD_EPR_AVS_ENABLE)
        if(srcCap->dat[pdo_no - 1u].pps_src.apdoType == CY_PDSTACK_APDO_AVS)
        {
            /* Output voltage in 25 mV unit */
            snkRdo.rdo_epr_avs.outVolt = glAppContractVoltage[port] * 2u;
            /* Operating current in 50 mA unit */
            snkRdo.rdo_epr_avs.opCur = (glAppContractPower[port] * 100u) / glAppContractVoltage[port];
        }
#endif /* CY_PD_EPR_AVS_ENABLE */
    }

#if (CY_PD_REV3_ENABLE)
    /* Supports unchunked extended messages in PD 3.0 mode. */
    if (dpm->specRevSopLive >= CY_PD_REV3)
    {
        snkRdo.rdo_gen.unchunkSup = true;
#if (CY_PD_EPR_ENABLE)
        snkRdo.rdo_gen.eprModeCapable = dpmExtStat->epr.snkEnable;
#endif /* CY_PD_EPR_ENABLE */
    }
#endif /* (CY_PD_REV3_ENABLE) */

    return snkRdo;
}

/*
 * Evaluate the source capabilities listed by the source and picks the appropriate one to request.
 */
void Cy_App_Pdo_EvalSrcCap(cy_stc_pdstack_context_t* context, const cy_stc_pdstack_pd_packet_t* srcCap, cy_pdstack_app_resp_cbk_t app_resp_handler)
{
    uint8_t src_pdo_index, snk_pdo_index;
    cy_pd_pd_do_t* snkPdo = (cy_pd_pd_do_t*)&context->dpmStat.curSnkPdo[0];
    uint8_t port = context->port;
    cy_stc_pdstack_dpm_status_t *dpm = &(context->dpmStat);
    uint16_t src_vsafe5_cur = srcCap->dat[0].fixed_src.maxCurrent; /* Source max current for first PDO */
    cy_pd_pd_do_t snkRdo;
    uint32_t highest_gl_contract_power = 0u;
    bool match = false;
    uint8_t src_pdo_len = srcCap->len;
    uint8_t snk_pdo_len = dpm->curSnkPdocount;
#if (CY_PD_EPR_ENABLE)
    cy_stc_pdstack_dpm_ext_status_t *dpmExt = &(context->dpmExtStat);
    bool eprActive = false;
    bool eprSpr = false;

    if(srcCap->hdr.hdr.extd)
    {
        src_pdo_len = srcCap->hdr.hdr.dataSize >> 2u;

        if(dpmExt->eprActive)
        {
            Cy_PdStack_Dpm_IsEprSpr(context, &eprSpr);
            if((src_pdo_len > CY_PD_MAX_NO_OF_PDO) && !eprSpr)
            {
                snk_pdo_len = CY_PD_MAX_NO_OF_PDO + dpmExt->curEprSnkPdoCount;
            }
            Cy_PdStack_Dpm_ChangeEprToSpr(context, false);
        }
    }
#endif /* CY_PD_EPR_ENABLE */

    for(snk_pdo_index = 0u; snk_pdo_index < snk_pdo_len; snk_pdo_index++)
    {
        for(src_pdo_index = 0u; src_pdo_index < src_pdo_len; src_pdo_index++)
        {
#if (CY_PD_EPR_ENABLE)
            if((src_pdo_index >= CY_PD_MAX_NO_OF_PDO) && (srcCap->dat[src_pdo_index].fixed_src.supplyType == CY_PDSTACK_PDO_BATTERY))
            {
                continue;
            }
#endif /* CY_PD_EPR_ENABLE */
            if(is_src_acceptable_snk(context, (cy_pd_pd_do_t*)(&srcCap->dat[src_pdo_index]), snk_pdo_index))
            {
                bool max_cond = false; 
                /*
                 * Support four different algorithms based on which the most appropriate source PDO is selected:
                 * CY_PDSTACK_HIGHEST_POWER   : Pick the fixed source PDO which delivers maximum amount of power
                 * CY_PDSTACK_HIGHEST_VOLTAGE : Pick the fixed source PDO which delivers power at maximum voltage
                 * CY_PDSTACK_HIGHEST_CURRENT : Pick the fixed source PDO which delivers the maximum current
                 * Default (none of the above): Pick the source PDO which delivers maximum amount of power
                 */
                switch(CY_APP_PD_PDO_SEL_ALGO)
                {
                    case CY_PDSTACK_HIGHEST_POWER:
                        /* Contract_power is based on SRC PDO */
                        if (srcCap->dat[src_pdo_index].fixed_src.supplyType == CY_PDSTACK_PDO_FIXED_SUPPLY)
                        {
                            uint32_t temp_power = calc_power(srcCap->dat[src_pdo_index].fixed_src.voltage,
                                    srcCap->dat[src_pdo_index].fixed_src.maxCurrent);
                            if (temp_power >= highest_gl_contract_power)
                            {
                                highest_gl_contract_power = temp_power;
                                max_cond = true;
                            }
                        }
                        break;

                    case CY_PDSTACK_HIGHEST_VOLTAGE:
                        /* Only fixed PDO takes part */
                        if ((srcCap->dat[src_pdo_index].fixed_src.supplyType == CY_PDSTACK_PDO_FIXED_SUPPLY)
                                && (glAppContractVoltage[port] >= highest_gl_contract_power))
                        {
                            highest_gl_contract_power = glAppContractVoltage[port];
                            max_cond = true;
                        }
                        break;

                    case CY_PDSTACK_HIGHEST_CURRENT:
                        /* Only fixed PDO takes part */
                        if ((srcCap->dat[src_pdo_index].fixed_src.supplyType == CY_PDSTACK_PDO_FIXED_SUPPLY)
                                && (srcCap->dat[src_pdo_index].fixed_src.maxCurrent >= highest_gl_contract_power))
                        {
                            highest_gl_contract_power = srcCap->dat[src_pdo_index].fixed_src.maxCurrent;
                            max_cond = true;
                        }
                        break;

                    default:
                        /* Contract_power is calculated in is_src_acceptable_snk() */
                        if (glAppContractPower[port] >= highest_gl_contract_power)
                        {
                            highest_gl_contract_power = glAppContractPower[port];
                            max_cond = true;
                        }
                        break;
                }

                if (max_cond)
                {
                    /* Check if sink needs higher capability */
                    if ((snkPdo[0].fixed_snk.highCap) && (glAppContractVoltage[port] == (CY_PD_VSAFE_5V/CY_PD_VOLT_PER_UNIT)))
                    {
                        /* 5 V contract is not acceptable with highCap = 1 */
                        continue;
                    }

                    snkRdo = form_rdo(context, (src_pdo_index + 1u), false,
                            (context->dpmStat.curSnkMaxMin[snk_pdo_index] & CY_PD_GIVE_BACK_MASK), srcCap);
                    match = true;
                }
            }
#if (CY_PD_EPR_ENABLE)
            /* Sink should not process received EPR_Source_Capabilites message with a
             * PDO greater than 100 W in any of the first seven object positions.
             * 100000 mW in 250 mW unit. */
            Cy_PdStack_Dpm_IsEprModeActive(context, &eprActive);
            if(eprActive && (srcCap->hdr.hdr.extd)
                && (src_pdo_index < CY_PD_MAX_NO_OF_PDO) && (glAppPdoPower[port] > 100000u/250u))
            {
                (Cy_App_GetRespBuffer(port))->reqStatus = CY_PDSTACK_REQ_SEND_HARD_RESET;
                app_resp_handler(context, Cy_App_GetRespBuffer(port));
                return;
            }
            else
            {
                /* Clear req_status */
                (Cy_App_GetRespBuffer(port))->reqStatus = (cy_en_pdstack_app_req_status_t)0;
            }
#endif /* CY_PD_EPR_ENABLE */
        }
    }

    if(match == false)
    {
        /* Capability mismatch: requests for vsafe5v PDO with CapMismatch */
        glAppContractVoltage[port] = snkPdo[0].fixed_snk.voltage;
        glAppOperCurPower[port] = snkPdo[0].fixed_snk.opCurrent;
        glAppContractPower[port] = CY_PDUTILS_DIV_ROUND_UP(
                glAppContractVoltage[port] * glAppOperCurPower[port], 500u);

        if(src_vsafe5_cur < glAppOperCurPower[port])
        {
            /* SNK operation current can not be bigger than SRC maxCurrent */
            glAppOperCurPower[port] = src_vsafe5_cur;
        }

        glAppMaxMinPower[port] = context->dpmStat.curSnkMaxMin[0];
        snkRdo = form_rdo(context, 1u, true, false, srcCap);
    }

    (Cy_App_GetRespBuffer(port))->respDo = snkRdo;
    app_resp_handler(context, Cy_App_GetRespBuffer(context->port));
}
#endif /* (!(CY_PD_SOURCE_ONLY)) */

#if (!CY_PD_SINK_ONLY)
/*
 * This function can be used to ask EC to evaluate a request message.
 * For now evaluating here and executing the callback in this function itself.
 */
void Cy_App_Pdo_EvalRdo(cy_stc_pdstack_context_t* context, cy_pd_pd_do_t rdo, cy_pdstack_app_resp_cbk_t app_resp_handler)
{
    uint8_t port = context->port;
#if (CY_PD_REV3_ENABLE && (!CY_PD_EPR_ENABLE))
    /* If EPR is not supported and the MSB of RDO is set in a PD 3.0 contract, trigger hard reset. */
    if ((context->dpmConfig.specRevSopLive >= CY_PD_REV3) && ((rdo.val & 0x80000000u) != 0))
    {
        Cy_App_GetRespBuffer(port)->reqStatus = CY_PDSTACK_REQ_SEND_HARD_RESET;
        app_resp_handler(context, Cy_App_GetRespBuffer(port));
        return;
    }
#endif /* CY_PD_REV3_ENABLE && (!CY_PD_EPR_ENABLE) */

    if (Cy_PdStack_Dpm_IsRdoValid(context, rdo) == CY_PDSTACK_STAT_SUCCESS)
    {
        Cy_App_GetRespBuffer(port)->reqStatus = CY_PDSTACK_REQ_ACCEPT;
    }
    else
    {
        Cy_App_GetRespBuffer(port)->reqStatus = CY_PDSTACK_REQ_REJECT;
    }

    app_resp_handler(context, Cy_App_GetRespBuffer(port));
}
#endif /* (!CY_PD_SINK_ONLY) */

/* [] END OF FILE */

