/*
 * Copyright (C) 1994-2020 Altair Engineering, Inc.
 * For more information, contact Altair at www.altair.com.
 *
 * This file is part of both the OpenPBS software ("OpenPBS")
 * and the PBS Professional ("PBS Pro") software.
 *
 * Open Source License Information:
 *
 * OpenPBS is free software. You can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * OpenPBS is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Commercial License Information:
 *
 * PBS Pro is commercially licensed software that shares a common core with
 * the OpenPBS software.  For a copy of the commercial license terms and
 * conditions, go to: (http://www.pbspro.com/agreement.html) or contact the
 * Altair Legal Department.
 *
 * Altair's dual-license business model allows companies, individuals, and
 * organizations to create proprietary derivative works of OpenPBS and
 * distribute them - whether embedded or bundled with other software -
 * under a commercial license agreement.
 *
 * Use of Altair's trademarks, including but not limited to "PBS™",
 * "OpenPBS®", "PBS Professional®", and "PBS Pro™" and Altair's logos is
 * subject to Altair's trademark licensing policies.
 */

/**
 * @file pbs_rstat.c
 */
#include <pbs_config.h>   /* the master config generated by configure */
#include <pbs_version.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cmds.h"
#include "pbs_ifl.h"


#define DISP_RESV_FULL		0x01	/* -F,-f option - full verbose description */
#define DISP_RESV_NAMES		0x02	/* -B option - Reservation names only */
#define DISP_RESV_DEFAULT		0x04	/* -S option - Default, Short Description */
#define DISP_INCR_WIDTH		0x08	/*  Increases the header width */

/* prototypes */
char *convert_resv_state(char *pcode, int long_str);
void handle_resv(char *resv_id, char *server, int how);
static int check_width;

/**
 * @brief
 *	display_single_reservation - display a single reservation
 *
 * @param[in] resv - the reservation to display
 * @param[in] how - 1: long form (all resv info)
 * 		      2: print reservation name
 *		      4: short form
 *                  8: increase header width
 * @return Void
 *
 */
void
display_single_reservation(struct batch_status *resv, int how)
{
	char		*queue_name = NULL;
#ifdef NAS /* localmod 075 */
	char		*resv_name = NULL;
#endif /* localmod 075 */
	char		*user = NULL;
	char		*resv_state = NULL;
	char		*resv_start = NULL;
	char		*resv_end = NULL;
	time_t		resv_duration = 0;
	char		*str;
	struct attrl	*attrp = NULL;
	time_t		tmp_time;
	char		tbuf[64];
	char		*fmt = "%a %b %d %H:%M:%S %Y";
	attrp = resv->attribs;

	if (how & DISP_RESV_NAMES) {			/* display just name of the reservation */
		printf("Resv ID: %s\n", resv->name);
	} else if (how & DISP_RESV_DEFAULT) {		/* display short form, default*/
		while (attrp != NULL) {
			if (strcmp(attrp->name, ATTR_queue) == 0)
				queue_name = attrp->value;
			else if (strcmp(attrp->name, ATTR_auth_u) == 0)
				user = attrp->value;
			else if (strcmp(attrp->name, ATTR_resv_start) == 0)
				resv_start = attrp->value;
			else if (strcmp(attrp->name, ATTR_resv_end) == 0)
				resv_end = attrp->value;
			else if (strcmp(attrp->name, ATTR_resv_duration) == 0)
				resv_duration = strtol(attrp->value, NULL, 10);
			else if (strcmp(attrp->name, ATTR_resv_state) == 0)
				resv_state = convert_resv_state(attrp->value, 0);/*short state str*/
#ifdef NAS /* localmod 075 */
			else if (strcmp(attrp->name, ATTR_resv_name) == 0) {
				if (strcmp(attrp->value, "NULL") != 0)
					resv_name = attrp->value;
			}
#endif /* localmod 075 */

			attrp = attrp->next;
		}
		if (how & DISP_INCR_WIDTH) {
			printf("%-15.15s %-13.13s %-8.8s %-5.5s ",
#ifdef NAS /* localmod 075 */
				(resv_name ? resv_name : resv->name),
				queue_name, user, resv_state);
#else
				resv->name, queue_name, user, resv_state);
#endif
		} else {
			printf("%-10.10s %-8.8s %-8.8s %-5.5s ",
#ifdef NAS /* localmod 075 */
				(resv_name ? resv_name : resv->name),
				queue_name, user, resv_state);
#else
				resv->name, queue_name, user, resv_state);
#endif /* localmod 075 */
		}
		printf("%17.17s / ", convert_time(resv_start));
		printf("%ld / %-17.17s\n", (long)resv_duration, convert_time(resv_end));
	} else {			/*display long form (all reservation info)*/
		printf("Resv ID: %s\n", resv->name);
		while (attrp != NULL) {
			if (attrp->resource != NULL)
				printf("%s.%s = %s\n", attrp->name, attrp->resource, show_nonprint_chars(attrp->value));
			else {
				if (strcmp(attrp->name, ATTR_resv_state) == 0) {
					str = convert_resv_state(attrp->value, 1);  /* long state str */
				} else if (strcmp(attrp->name, ATTR_resv_start) == 0 ||
					strcmp(attrp->name, ATTR_resv_end) == 0    ||
					strcmp(attrp->name, ATTR_ctime) == 0      ||
					strcmp(attrp->name, ATTR_mtime) == 0      ||
					strcmp(attrp->name, ATTR_resv_retry) == 0) {
					tmp_time = atol(attrp->value);
					strftime(tbuf, sizeof(tbuf), fmt, localtime((time_t *) &tmp_time));
					str = tbuf;
				} else if (!strcmp(attrp->name, ATTR_resv_execvnodes)) {
					attrp = attrp->next;
					continue;
				} else if (!strcmp(attrp->name, ATTR_resv_standing)) {
					attrp = attrp->next;
					continue;
				} else if (!strcmp(attrp->name, ATTR_resv_timezone)) {
					attrp = attrp->next;
					continue;
				} else if (!strcmp(attrp->name, ATTR_resv_count)) {
					str = attrp->value;
				} else if (!strcmp(attrp->name, ATTR_resv_rrule)) {
					str = attrp->value;
				} else if (!strcmp(attrp->name, ATTR_resv_idx)) {
					str = attrp->value;
				} else {
					str = attrp->value;
				}
				printf("%s = %s\n", attrp->name, show_nonprint_chars(str));
			}
			attrp = attrp->next;
		}
		printf("\n");
	}
}


/**
 * @brief
 *	display - display the resv data
 *
 * @param[in] bstat - the batch_status list to display
 * @param[in] how - 1: long form (all resv info)
 *		      2: print reservation name
 *		      4: short form
 *		      8: increase header width
 *
 *@return Void
 *
 */
void
display(struct batch_status *resv, int how)
{
	struct batch_status *cur;	/* loop var - current batch_status in loop */
	static char no_display = 0;

	if (resv == NULL)
		return;

	cur = resv;

	if ((how & DISP_RESV_DEFAULT) && (!no_display)) {
		if (how & DISP_INCR_WIDTH) {
		        printf("%-15.15s %-13.13s %-8.8s %-5.5s %17.17s / Duration / %-17.17s\n",
#ifdef NAS /* localmod 075 ( */
					"Name", "Queue", "User", "State", "Start", "End");
#else
					"Resv ID", "Queue", "User", "State", "Start", "End");
#endif /* localmod 075 */
		        printf("-------------------------------------------------------------------------------\n");
		} else {
			printf("%-10.10s %-8.8s %-8.8s %-5.5s %17.17s / Duration / %-17.17s\n",
#ifdef NAS /* localmod 075 ( */
					"Name", "Queue", "User", "State", "Start", "End");
#else
					"Resv ID", "Queue", "User", "State", "Start", "End");
#endif /* localmod 075 */
			printf("---------------------------------------------------------------------\n");
		}

		/* only display header once */
		no_display = 1;
	}

	while (cur != NULL) {
		display_single_reservation(cur, how);
		cur = cur->next;
	}
}

int
main(int argc, char *argv[])
{
	int c;			/* for getopts() */
	int how = DISP_RESV_DEFAULT; /* how the reservation should be display, default to short listing */
	int errflg = 0;
	int i;
	char *resv_id;		/* reservation ID from the command line */
	char resv_id_out[PBS_MAXCLTJOBID];
	char server_out[MAXSERVERNAME];
	/*test for real deal or just version and exit*/

	PRINT_VERSION_AND_EXIT(argc, argv);

	if (initsocketlib())
		return 1;

	while ((c = getopt(argc, argv, "fFBS")) != EOF) {
		switch (c) {
			case 'F':			/* full verbose description */
			case 'f':
				how = DISP_RESV_FULL;
				break;

			case 'B':			/* Brief, just the names */
				how = DISP_RESV_NAMES;
				break;

			case 'S':			/* short desc, default */
				how = DISP_RESV_DEFAULT;
				break;

			default:
				errflg = 1;
		}
	}

	if (errflg) {
		fprintf(stderr, "Usage:\n\tpbs_rstat [-fFBS] [reservation-id]\n");
		fprintf(stderr, "\tpbs_rstat --version\n");
		exit(1);
	}

	if (CS_client_init() != CS_SUCCESS) {
		fprintf(stderr, "pbs_rstat: unable to initialize security library.\n");
		exit(1);
	}

	if (optind == argc)
		handle_resv(NULL, NULL, how);
	else {
		for (i = optind; i < argc; i++) {
			resv_id = argv[i];

#ifdef NAS /* localmod 075 */
			if (*resv_id == '@') {
				handle_resv(NULL, resv_id+1, how);
				continue;
			} else if (get_server(resv_id, resv_id_out, server_out)) {
#else
			if (get_server(resv_id, resv_id_out, server_out)) {
#endif /* localmod 075 */
				fprintf(stderr,
					"pbs_rstat: illegally formed reservation identifier: %s\n", resv_id);
				errflg = 1;
#ifdef NAS /* localmod 075 */
				continue;
			}
#else
			} else
#endif /* localmod 075 */
				handle_resv(resv_id_out, server_out, how);
		}
	}
	CS_close_app();
	exit(errflg);
}

/*
 * @brief
 *	handle_resv - handle connecting to the server, and displaying the
 *			reservations
 *
 * @param[in] resv_id - the id of the reservation
 * @param[in] server  - the server to connect to
 * @param[in] how     - 0: just the names
 *                      1: short display
 *                      2: full display (all attributes)
 *
 * @return Void
 *
 */
void
handle_resv(char *resv_id, char *server, int how)
{
	int pbs_sd;
	struct batch_status *bstat;
	char *errmsg;
	/* for dynamic pbs_rstat width */
	struct batch_status *server_attrs;

	pbs_sd = cnt2server(server);

	if (pbs_sd < 0) {
		fprintf(stderr, "pbs_rstat: cannot connect to server (errno=%d)\n",
			pbs_errno);
		CS_close_app();
		exit(pbs_errno);
	}
	/* check the server attribute max_job_sequence_id value */
	if (check_width == 0) {
		server_attrs = pbs_statserver(pbs_sd, NULL, NULL);
		if (server_attrs == NULL && pbs_errno != PBSE_NONE) {
			if ((errmsg = pbs_geterrmsg(pbs_sd)) != NULL)
				fprintf(stderr, "pbs_rstat: %s\n", errmsg);
			else
				fprintf(stderr, "pbs_rstat: Error %d\n", pbs_errno);
			return;
		}

		if (server_attrs != NULL) {
			int check_seqid_len;
			check_seqid_len = check_max_job_sequence_id(server_attrs);
			if (check_seqid_len == 1) {
				how |= DISP_INCR_WIDTH; /* increased column width*/
			}
			pbs_statfree(server_attrs);
			server_attrs = NULL;
			check_width = 1;
		}
	}

	bstat = pbs_statresv(pbs_sd, resv_id, NULL, NULL);

	if (pbs_errno) {
		errmsg = pbs_geterrmsg(pbs_sd);
		fprintf(stderr, "pbs_rstat: %s\n", errmsg);
	}

	display(bstat, how);

}

/*
 * @brief
 *	convert_resv_state - convert the reservation state from a
 *			     string integer enum resv_states value into
 *			     a human readable string
 *
 * @param[out] pcode - the string enum value
 * @param[in] long_str - int value to indicate short or long human readable string to be printed
 *
 * @return - string
 * @retval   "state of reservation"
 *
 */
char *
convert_resv_state(char *pcode, int long_str)
{
	int i;
	static char *resv_strings_short[] =
		{ "NO", "UN", "CO", "WT", "TR", "RN", "FN", "BD", "DE", "DJ", "DG", "AL", "IC" };
	static char *resv_strings_long[] =
		{ "RESV_NONE", "RESV_UNCONFIRMED", "RESV_CONFIRMED",
		"RESV_WAIT", "RESV_TIME_TO_RUN", "RESV_RUNNING",
		"RESV_FINISHED", "RESV_BEING_DELETED", "RESV_DELETED",
		"RESV_DELETING_JOBS", "RESV_DEGRADED", "RESV_BEING_ALTERED",
		"RESV_IN_CONFLICT"
	};

	i = atoi(pcode);
	switch (i) {
		case RESV_NONE:
		case RESV_UNCONFIRMED:
		case RESV_CONFIRMED:
		case RESV_DEGRADED:
		case RESV_WAIT:
		case RESV_TIME_TO_RUN:
		case RESV_RUNNING:
		case RESV_FINISHED:
		case RESV_BEING_DELETED:
		case RESV_DELETED:
		case RESV_DELETING_JOBS:
		case RESV_BEING_ALTERED:
		case RESV_IN_CONFLICT:
			if (long_str == 0)	/* short */
				return resv_strings_short[i];
			else
				return resv_strings_long[i];
	}
	return  pcode;
}
